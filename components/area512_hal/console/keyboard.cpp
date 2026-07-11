// Cardputer onboard keyboard: scans the TCA8418 I2C key matrix, applies
// modifier/Fn/Ctrl mapping and auto-repeat, and runs a romaji->kana IME.
// Decoded bytes are pushed into the console stdin queue.
#include "console_internal.h"

#include <M5Unified.hpp>

#include <stdint.h>
#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "keyboard";

static constexpr uint8_t TCA8418_ADDR = 0x34;
static constexpr uint32_t TCA8418_FREQ = 400000;
static constexpr uint8_t TCA8418_REG_CFG = 0x01;
static constexpr uint8_t TCA8418_REG_INT_STAT = 0x02;
static constexpr uint8_t TCA8418_KEY_EVENT_A = 0x04;
static constexpr uint8_t TCA8418_REG_KP_GPIO1 = 0x1D;
static constexpr uint8_t TCA8418_REG_KP_GPIO2 = 0x1E;
static constexpr uint8_t TCA8418_REG_KP_GPIO3 = 0x1F;
static constexpr uint8_t TCA8418_CFG_AI = 0x80;
static constexpr uint8_t TCA8418_CFG_KE_IEN = 0x01;
static constexpr uint8_t TCA8418_INT_KEY = 0x01;
static constexpr uint8_t TCA8418_INT_OVERFLOW = 0x08;
static constexpr uint8_t TCA8418_KEY_CODE_MASK = 0x7F;
static constexpr uint8_t TCA8418_KEY_PRESSED_BIT = 0x80;
static constexpr int TCA8418_FIFO_DEPTH = 10;
static constexpr int TCA8418_RECOVER_AFTER_ERRORS = 8;
static constexpr int CARDPUTER_REPEAT_DELAY_MS = 450;
static constexpr int CARDPUTER_REPEAT_INTERVAL_MS = 55;
static constexpr int CARDPUTER_REPEAT_TIMEOUT_MS = 2000;

static constexpr uint8_t ASCII_BACKSPACE = 0x08;
static constexpr uint8_t ASCII_ESCAPE = 0x1B;
static constexpr uint8_t ASCII_DELETE = 0x7F;

static bool s_ime_active = false;
static char s_ime_buffer[8];
static int s_ime_buffer_length = 0;

namespace {
struct RomajiEntry {
  const char *romaji;
  const char *kana;
};
} // namespace

static const RomajiEntry ROMAJI_TABLE[] = {
  {"nn", "ん"},    {"n'", "ん"},    {"xn", "ん"},    {"a", "あ"},
  {"i", "い"},     {"u", "う"},     {"e", "え"},     {"o", "お"},
  {"ka", "か"},    {"ki", "き"},    {"ku", "く"},    {"ke", "け"},
  {"ko", "こ"},    {"kya", "きゃ"}, {"kyi", "きぃ"}, {"kyu", "きゅ"},
  {"kye", "きぇ"}, {"kyo", "きょ"}, {"ga", "が"},    {"gi", "ぎ"},
  {"gu", "ぐ"},    {"ge", "げ"},    {"go", "ご"},    {"gya", "ぎゃ"},
  {"gyu", "ぎゅ"}, {"gyo", "ぎょ"}, {"sa", "さ"},    {"si", "し"},
  {"shi", "し"},   {"su", "す"},    {"se", "せ"},    {"so", "そ"},
  {"sha", "しゃ"}, {"sya", "しゃ"}, {"shu", "しゅ"}, {"syu", "しゅ"},
  {"she", "しぇ"}, {"sye", "しぇ"}, {"sho", "しょ"}, {"syo", "しょ"},
  {"za", "ざ"},    {"zi", "じ"},    {"ji", "じ"},    {"zu", "ず"},
  {"ze", "ぜ"},    {"zo", "ぞ"},    {"ja", "じゃ"},  {"zya", "じゃ"},
  {"jya", "じゃ"}, {"ju", "じゅ"},  {"zyu", "じゅ"}, {"jyu", "じゅ"},
  {"je", "じぇ"},  {"zye", "じぇ"}, {"jye", "じぇ"}, {"jo", "じょ"},
  {"zyo", "じょ"}, {"jyo", "じょ"}, {"ta", "た"},    {"ti", "ち"},
  {"chi", "ち"},   {"tu", "つ"},    {"tsu", "つ"},   {"te", "て"},
  {"to", "と"},    {"cha", "ちゃ"}, {"tya", "ちゃ"}, {"chu", "ちゅ"},
  {"tyu", "ちゅ"}, {"che", "ちぇ"}, {"tye", "ちぇ"}, {"cho", "ちょ"},
  {"tyo", "ちょ"}, {"tha", "てゃ"}, {"thi", "てぃ"}, {"thu", "てゅ"},
  {"the", "てぇ"}, {"tho", "てょ"}, {"da", "だ"},    {"di", "ぢ"},
  {"du", "づ"},    {"de", "で"},    {"do", "ど"},    {"dya", "ぢゃ"},
  {"dyu", "ぢゅ"}, {"dyo", "ぢょ"}, {"dha", "でゃ"}, {"dhi", "でぃ"},
  {"dhu", "でゅ"}, {"dhe", "でぇ"}, {"dho", "でょ"}, {"na", "な"},
  {"ni", "に"},    {"nu", "ぬ"},    {"ne", "ね"},    {"no", "の"},
  {"nya", "にゃ"}, {"nyu", "にゅ"}, {"nyo", "にょ"}, {"ha", "は"},
  {"hi", "ひ"},    {"hu", "ふ"},    {"fu", "ふ"},    {"he", "へ"},
  {"ho", "ほ"},    {"hya", "ひゃ"}, {"hyu", "ひゅ"}, {"hyo", "ひょ"},
  {"fa", "ふぁ"},  {"fi", "ふぃ"},  {"fe", "ふぇ"},  {"fo", "ふぉ"},
  {"ba", "ば"},    {"bi", "び"},    {"bu", "ぶ"},    {"be", "べ"},
  {"bo", "ぼ"},    {"bya", "びゃ"}, {"byu", "びゅ"}, {"byo", "びょ"},
  {"pa", "ぱ"},    {"pi", "ぴ"},    {"pu", "ぷ"},    {"pe", "ぺ"},
  {"po", "ぽ"},    {"pya", "ぴゃ"}, {"pyu", "ぴゅ"}, {"pyo", "ぴょ"},
  {"ma", "ま"},    {"mi", "み"},    {"mu", "む"},    {"me", "め"},
  {"mo", "も"},    {"mya", "みゃ"}, {"myu", "みゅ"}, {"myo", "みょ"},
  {"ya", "や"},    {"yu", "ゆ"},    {"yo", "よ"},    {"ra", "ら"},
  {"ri", "り"},    {"ru", "る"},    {"re", "れ"},    {"ro", "ろ"},
  {"rya", "りゃ"}, {"ryu", "りゅ"}, {"ryo", "りょ"}, {"wa", "わ"},
  {"wi", "うぃ"},  {"we", "うぇ"},  {"wo", "を"},    {"xa", "ぁ"},
  {"xi", "ぃ"},    {"xu", "ぅ"},    {"xe", "ぇ"},    {"xo", "ぉ"},
  {"la", "ぁ"},    {"li", "ぃ"},    {"lu", "ぅ"},    {"le", "ぇ"},
  {"lo", "ぉ"},    {"xya", "ゃ"},   {"xyu", "ゅ"},   {"xyo", "ょ"},
  {"lya", "ゃ"},   {"lyu", "ゅ"},   {"lyo", "ょ"},   {"xtu", "っ"},
  {"xtsu", "っ"},  {"ltu", "っ"},   {"ltsu", "っ"},  {"xwa", "ゎ"},
  {"lwa", "ゎ"},   {"-", "ー"},
};

static constexpr int ROMAJI_TABLE_SIZE =
  sizeof(ROMAJI_TABLE) / sizeof(ROMAJI_TABLE[0]);

static bool
ime_is_vowel(char character) {
  return character == 'a' || character == 'i' || character == 'u' ||
         character == 'e' || character == 'o';
}

static const RomajiEntry *
ime_find_matching_entry(bool *prefix_only) {
  const RomajiEntry *match = nullptr;
  *prefix_only = false;

  for (int i = 0; i < ROMAJI_TABLE_SIZE; ++i) {
    const char *romaji = ROMAJI_TABLE[i].romaji;

    if (strcmp(s_ime_buffer, romaji) == 0) {
      match = &ROMAJI_TABLE[i];
      continue;
    }

    if (strncmp(s_ime_buffer, romaji, s_ime_buffer_length) == 0 &&
        romaji[s_ime_buffer_length] != '\0') {
      *prefix_only = true;
    }
  }

  return match;
}

static void
ime_push_kana(const char *kana) {
  while (*kana) {
    area512_console_stdin_push((uint8_t)*kana++);
  }
}

static void
ime_consume(char character) {
  if (s_ime_buffer_length >= (int)sizeof(s_ime_buffer) - 1) {
    s_ime_buffer_length = 0;
  }

  s_ime_buffer[s_ime_buffer_length++] = character;
  s_ime_buffer[s_ime_buffer_length] = '\0';

  while (s_ime_buffer_length > 0) {
    if (s_ime_buffer_length >= 2 && s_ime_buffer[0] == 'n' &&
        !ime_is_vowel(s_ime_buffer[1]) && s_ime_buffer[1] != 'y' &&
        s_ime_buffer[1] != 'n' && s_ime_buffer[1] != '\'') {

      ime_push_kana("ん");
      memmove(s_ime_buffer, s_ime_buffer + 1, s_ime_buffer_length);
      --s_ime_buffer_length;

      continue;
    }

    if (s_ime_buffer_length >= 2 && s_ime_buffer[0] == s_ime_buffer[1] &&
        !ime_is_vowel(s_ime_buffer[0]) && s_ime_buffer[0] != 'n') {

      ime_push_kana("っ");
      memmove(s_ime_buffer, s_ime_buffer + 1, s_ime_buffer_length);
      --s_ime_buffer_length;

      continue;
    }

    bool prefix_only = false;
    const RomajiEntry *match = ime_find_matching_entry(&prefix_only);

    if (match && !prefix_only) {
      ime_push_kana(match->kana);
      s_ime_buffer_length = 0;
      s_ime_buffer[0] = '\0';

      return;
    }

    if (prefix_only) {
      return;
    }

    // Drop the unmatchable leading byte and retry. Length counts the trailing
    // NUL too, so the terminator moves with it.
    memmove(s_ime_buffer, s_ime_buffer + 1, s_ime_buffer_length);
    --s_ime_buffer_length;
  }
}

static void
ime_flush(void) {
  // A lone 'n' has no chance to settle into "ん" on its own, so commit it here;
  // any other pending prefix is an incomplete romaji and is dropped.
  if (s_ime_buffer_length == 1 && s_ime_buffer[0] == 'n') {
    ime_push_kana("ん");
  }

  s_ime_buffer_length = 0;
  s_ime_buffer[0] = '\0';
}

static bool
tca8418_write_register(uint8_t register_address, uint8_t value) {
  if (!M5.In_I2C.start(TCA8418_ADDR, false, TCA8418_FREQ)) {
    return false;
  }

  uint8_t buffer[2] = {register_address, value};
  bool ok = M5.In_I2C.write(buffer, 2);

  M5.In_I2C.stop();

  return ok;
}

static bool
tca8418_read_register(uint8_t register_address, uint8_t *value_out) {
  if (value_out == nullptr) {
    return false;
  }

  if (!M5.In_I2C.start(TCA8418_ADDR, false, TCA8418_FREQ)) {
    return false;
  }

  bool ok = M5.In_I2C.write(&register_address, 1);

  M5.In_I2C.stop();

  if (!ok) {
    return false;
  }

  if (!M5.In_I2C.start(TCA8418_ADDR, true, TCA8418_FREQ)) {
    return false;
  }

  ok = M5.In_I2C.read(value_out, 1, true);

  M5.In_I2C.stop();

  return ok;
}

namespace {
enum class CardputerKeyKind : uint8_t {
  None,
  Char,
  Shift,
  Fn,
  Ctrl,
  Opt,
  Alt,
  Enter,
  Tab,
  Backspace,
  Space
};

struct CardputerKey {
  CardputerKeyKind kind;
  char character;
};
} // namespace

static CardputerKey
cardputer_lookup_key(uint8_t code) {
  switch (code) {
  case 1:
    return {CardputerKeyKind::Char, '`'};
  case 2:
    return {CardputerKeyKind::Tab, 0};
  case 3:
    return {CardputerKeyKind::Fn, 0};
  case 4:
    return {CardputerKeyKind::Ctrl, 0};
  case 5:
    return {CardputerKeyKind::Char, '1'};
  case 6:
    return {CardputerKeyKind::Char, 'q'};
  case 7:
    return {CardputerKeyKind::Shift, 0};
  case 8:
    return {CardputerKeyKind::Opt, 0};
  case 11:
    return {CardputerKeyKind::Char, '2'};
  case 12:
    return {CardputerKeyKind::Char, 'w'};
  case 13:
    return {CardputerKeyKind::Char, 'a'};
  case 14:
    return {CardputerKeyKind::Alt, 0};
  case 15:
    return {CardputerKeyKind::Char, '3'};
  case 16:
    return {CardputerKeyKind::Char, 'e'};
  case 17:
    return {CardputerKeyKind::Char, 's'};
  case 18:
    return {CardputerKeyKind::Char, 'z'};
  case 21:
    return {CardputerKeyKind::Char, '4'};
  case 22:
    return {CardputerKeyKind::Char, 'r'};
  case 23:
    return {CardputerKeyKind::Char, 'd'};
  case 24:
    return {CardputerKeyKind::Char, 'x'};
  case 25:
    return {CardputerKeyKind::Char, '5'};
  case 26:
    return {CardputerKeyKind::Char, 't'};
  case 27:
    return {CardputerKeyKind::Char, 'f'};
  case 28:
    return {CardputerKeyKind::Char, 'c'};
  case 31:
    return {CardputerKeyKind::Char, '6'};
  case 32:
    return {CardputerKeyKind::Char, 'y'};
  case 33:
    return {CardputerKeyKind::Char, 'g'};
  case 34:
    return {CardputerKeyKind::Char, 'v'};
  case 35:
    return {CardputerKeyKind::Char, '7'};
  case 36:
    return {CardputerKeyKind::Char, 'u'};
  case 37:
    return {CardputerKeyKind::Char, 'h'};
  case 38:
    return {CardputerKeyKind::Char, 'b'};
  case 41:
    return {CardputerKeyKind::Char, '8'};
  case 42:
    return {CardputerKeyKind::Char, 'i'};
  case 43:
    return {CardputerKeyKind::Char, 'j'};
  case 44:
    return {CardputerKeyKind::Char, 'n'};
  case 45:
    return {CardputerKeyKind::Char, '9'};
  case 46:
    return {CardputerKeyKind::Char, 'o'};
  case 47:
    return {CardputerKeyKind::Char, 'k'};
  case 48:
    return {CardputerKeyKind::Char, 'm'};
  case 51:
    return {CardputerKeyKind::Char, '0'};
  case 52:
    return {CardputerKeyKind::Char, 'p'};
  case 53:
    return {CardputerKeyKind::Char, 'l'};
  case 54:
    return {CardputerKeyKind::Char, ','};
  case 55:
    return {CardputerKeyKind::Char, '_'};
  case 56:
    return {CardputerKeyKind::Char, '['};
  case 57:
    return {CardputerKeyKind::Char, ';'};
  case 58:
    return {CardputerKeyKind::Char, '.'};
  case 61:
    return {CardputerKeyKind::Char, '='};
  case 62:
    return {CardputerKeyKind::Char, ']'};
  case 63:
    return {CardputerKeyKind::Char, '\''};
  case 64:
    return {CardputerKeyKind::Char, '/'};
  case 65:
    return {CardputerKeyKind::Backspace, 0};
  case 66:
    return {CardputerKeyKind::Char, '\\'};
  case 67:
    return {CardputerKeyKind::Enter, 0};
  case 68:
    return {CardputerKeyKind::Space, 0};
  default:
    return {CardputerKeyKind::None, 0};
  }
}

static bool s_cardputer_shift = false;
static bool s_cardputer_fn = false;
static bool s_cardputer_ctrl = false;
static bool s_cardputer_opt = false;
static bool s_cardputer_alt = false;
static bool s_cardputer_key_down[TCA8418_KEY_CODE_MASK + 1] = {};
static uint8_t s_cardputer_repeat_code = 0;
static TickType_t s_cardputer_repeat_next_tick = 0;
static TickType_t s_cardputer_repeat_start_tick = 0;
static int s_cardputer_i2c_error_count = 0;

static void
cardputer_clear_repeat_tracking(void) {
  s_cardputer_repeat_code = 0;
  s_cardputer_repeat_next_tick = 0;
  s_cardputer_repeat_start_tick = 0;
}

static bool
cardputer_is_modifier_key(CardputerKeyKind kind) {
  switch (kind) {
  case CardputerKeyKind::Shift:
  case CardputerKeyKind::Fn:
  case CardputerKeyKind::Ctrl:
  case CardputerKeyKind::Opt:
  case CardputerKeyKind::Alt:
    return true;
  default:
    return false;
  }
}

static bool
cardputer_is_repeatable_key(CardputerKeyKind kind) {
  return kind != CardputerKeyKind::None && !cardputer_is_modifier_key(kind);
}

static void
cardputer_set_repeat_key(uint8_t code) {
  CardputerKey key = cardputer_lookup_key(code);

  if (!cardputer_is_repeatable_key(key.kind)) {
    return;
  }

  s_cardputer_repeat_code = code;
  s_cardputer_repeat_start_tick = xTaskGetTickCount();

  s_cardputer_repeat_next_tick =
    s_cardputer_repeat_start_tick + pdMS_TO_TICKS(CARDPUTER_REPEAT_DELAY_MS);
}

static void
cardputer_clear_repeat_key(uint8_t code) {
  if (s_cardputer_repeat_code != code) {
    return;
  }

  cardputer_clear_repeat_tracking();

  for (uint8_t i = 1; i <= TCA8418_KEY_CODE_MASK; ++i) {
    CardputerKey key = cardputer_lookup_key(i);

    if (s_cardputer_key_down[i] && cardputer_is_repeatable_key(key.kind)) {
      cardputer_set_repeat_key(i);
      return;
    }
  }
}

static char
cardputer_to_upper(char character) {
  if (character >= 'a' && character <= 'z') {
    return (char)(character - 'a' + 'A');
  }

  return character;
}

static char
cardputer_apply_shift(char character) {
  if (character >= 'a' && character <= 'z') {
    return cardputer_to_upper(character);
  }

  switch (character) {
  case '`':
    return '~';
  case '1':
    return '!';
  case '2':
    return '@';
  case '3':
    return '#';
  case '4':
    return '$';
  case '5':
    return '%';
  case '6':
    return '^';
  case '7':
    return '&';
  case '8':
    return '*';
  case '9':
    return '(';
  case '0':
    return ')';
  case '_':
    return '-';
  case '=':
    return '+';
  case '[':
    return '{';
  case ']':
    return '}';
  case ';':
    return ':';
  case '\'':
    return '"';
  case ',':
    return '<';
  case '.':
    return '>';
  case '/':
    return '?';
  case '\\':
    return '|';
  default:
    return character;
  }
}

static bool
cardputer_lookup_fn_sequence(char base, const char **sequence) {
  switch (base) {
  case ';':
    *sequence = "\x1b[A";
    return true;
  case '.':
    *sequence = "\x1b[B";
    return true;
  case ',':
    *sequence = "\x1b[D";
    return true;
  case '/':
    *sequence = "\x1b[C";
    return true;
  case '`':
    *sequence = "`";
    return true;
  default:
    return false;
  }
}

static void
cardputer_feed_ime(uint8_t byte) {
  if (byte == ASCII_BACKSPACE || byte == ASCII_DELETE) {
    if (s_ime_active && s_ime_buffer_length > 0) {
      s_ime_buffer[--s_ime_buffer_length] = '\0';
    } else {
      area512_console_stdin_push(byte);
    }

    return;
  }

  if (s_ime_active) {
    if (byte >= 'a' && byte <= 'z') {
      ime_consume((char)byte);
      return;
    }

    if (byte >= 'A' && byte <= 'Z') {
      ime_consume((char)(byte - 'A' + 'a'));
      return;
    }
    if (byte == '-') {
      ime_consume('-');
      return;
    }

    ime_flush();
  }

  area512_console_stdin_push(byte);
}

static void
cardputer_handle_key(uint8_t code, bool pressed) {
  CardputerKey key = cardputer_lookup_key(code);

  switch (key.kind) {
  case CardputerKeyKind::Shift:
    s_cardputer_shift = pressed;
    return;
  case CardputerKeyKind::Fn:
    s_cardputer_fn = pressed;
    return;
  case CardputerKeyKind::Ctrl:
    s_cardputer_ctrl = pressed;
    return;
  case CardputerKeyKind::Opt:
    s_cardputer_opt = pressed;
    return;
  case CardputerKeyKind::Alt:
    s_cardputer_alt = pressed;
    return;
  case CardputerKeyKind::None:
    return;
  default:
    break;
  }

  if (!pressed) {
    return;
  }

  switch (key.kind) {
  case CardputerKeyKind::Enter:
    ime_flush();
    area512_console_stdin_push('\r');

    return;

  case CardputerKeyKind::Tab:
    cardputer_feed_ime('\t');

    return;

  case CardputerKeyKind::Backspace:
    cardputer_feed_ime(ASCII_BACKSPACE);

    return;

  case CardputerKeyKind::Space:
    if (s_cardputer_ctrl) {
      ime_flush();

      s_ime_active = !s_ime_active;

      return;
    }

    cardputer_feed_ime(' ');

    return;

  case CardputerKeyKind::Char:
    break;

  default:
    return;
  }

  char base = key.character;

  if (s_cardputer_fn) {
    const char *sequence = nullptr;

    if (cardputer_lookup_fn_sequence(base, &sequence)) {
      ime_flush();

      area512_console_stdin_push_bytes(sequence);

      return;
    }
  }

  char character = s_cardputer_shift ? cardputer_apply_shift(base) : base;
  if (character == '`') {
    ime_flush();

    area512_console_stdin_push(ASCII_ESCAPE);

    return;
  }

  if (s_cardputer_ctrl) {
    char upper_character = cardputer_to_upper(character);

    if (upper_character >= 'A' && upper_character <= 'Z') {
      ime_flush();
      area512_console_stdin_push((uint8_t)(upper_character & 0x1F));

      return;
    }
  }

  if (s_cardputer_opt || s_cardputer_alt) {
    ime_flush();

    area512_console_stdin_push(ASCII_ESCAPE);
    area512_console_stdin_push((uint8_t)character);

    return;
  }

  cardputer_feed_ime((uint8_t)character);
}

static void
cardputer_update_repeat(void) {
  if (s_cardputer_repeat_code == 0 ||
      !s_cardputer_key_down[s_cardputer_repeat_code]) {

    return;
  }

  TickType_t now = xTaskGetTickCount();

  if ((int32_t)(now - s_cardputer_repeat_start_tick) >=
      (int32_t)pdMS_TO_TICKS(CARDPUTER_REPEAT_TIMEOUT_MS)) {

    s_cardputer_key_down[s_cardputer_repeat_code] = false;
    cardputer_clear_repeat_tracking();

    return;
  }

  if ((int32_t)(now - s_cardputer_repeat_next_tick) < 0) {
    return;
  }

  cardputer_handle_key(s_cardputer_repeat_code, true);

  s_cardputer_repeat_next_tick =
    now + pdMS_TO_TICKS(CARDPUTER_REPEAT_INTERVAL_MS);
}

// A failed I2C read can drop a key-up event, leaving s_cardputer_key_down
// latched and auto-repeat stuck. Clear only down/repeat state; modifiers are
// left alone.
static void
cardputer_clear_repeat_state(void) {
  memset(s_cardputer_key_down, 0, sizeof(s_cardputer_key_down));
  cardputer_clear_repeat_tracking();
}

static void
cardputer_reset_local_state(void) {
  s_cardputer_shift = false;
  s_cardputer_fn = false;
  s_cardputer_ctrl = false;
  s_cardputer_opt = false;
  s_cardputer_alt = false;
  cardputer_clear_repeat_state();
}

static bool
cardputer_configure_tca8418(void) {
  bool ok = true;

  // Each register is independent, so write them all even if one fails and just
  // report the result; a half-configured TCA8418 is worse than retrying later.
  ok = tca8418_write_register(TCA8418_REG_KP_GPIO1, 0xFF) && ok;
  ok = tca8418_write_register(TCA8418_REG_KP_GPIO2, 0xFF) && ok;
  ok = tca8418_write_register(TCA8418_REG_KP_GPIO3, 0x03) && ok;
  ok = tca8418_write_register(
         TCA8418_REG_CFG,
         (uint8_t)(TCA8418_CFG_AI | TCA8418_CFG_KE_IEN)
       ) &&
       ok;
  ok = tca8418_write_register(TCA8418_REG_INT_STAT, 0xFF) && ok;

  // Drain stale events left in the FIFO from before init so they don't surface
  // as phantom keypresses. +4 is slop over the FIFO depth; 0x00 means empty.
  for (int i = 0; i < TCA8418_FIFO_DEPTH + 4; ++i) {
    uint8_t event = 0;

    if (!tca8418_read_register(TCA8418_KEY_EVENT_A, &event) || event == 0x00) {
      break;
    }
  }

  tca8418_write_register(TCA8418_REG_INT_STAT, 0xFF);
  cardputer_reset_local_state();

  s_cardputer_i2c_error_count = 0;

  return ok;
}

static void
cardputer_handle_i2c_error(void) {
  if (++s_cardputer_i2c_error_count < TCA8418_RECOVER_AFTER_ERRORS) {
    return;
  }

  ESP_LOGW(TAG, "TCA8418 I2C read failed repeatedly; reinitializing keyboard");

  cardputer_configure_tca8418();
}

bool
keyboard_init(void) {
  uint8_t probe = 0;

  if (!tca8418_read_register(TCA8418_REG_CFG, &probe)) {
    ESP_LOGW(
      TAG,
      "TCA8418 not responding on M5.In_I2C (addr=0x%02x)",
      TCA8418_ADDR
    );

    return false;
  }

  return cardputer_configure_tca8418();
}

void
keyboard_poll(void) {
  uint8_t interrupt_status = 0;

  if (!tca8418_read_register(TCA8418_REG_INT_STAT, &interrupt_status)) {
    cardputer_clear_repeat_state();
    cardputer_handle_i2c_error();

    return;
  }

  bool i2c_ok = true;

  // The event-count register is unreliable; drain KEY_EVENT_A until it
  // reads 0 (FIFO empty), up to the FIFO depth.
  for (int i = 0; i < TCA8418_FIFO_DEPTH; ++i) {
    uint8_t event = 0;

    if (!tca8418_read_register(TCA8418_KEY_EVENT_A, &event)) {
      // This read may have dropped a key-up; clear repeat state before
      // bailing.
      cardputer_clear_repeat_state();

      i2c_ok = false;

      cardputer_handle_i2c_error();

      break;
    }

    if (event == 0x00) {
      break;
    }

    uint8_t code = event & TCA8418_KEY_CODE_MASK;
    bool pressed = (event & TCA8418_KEY_PRESSED_BIT) != 0;

    s_cardputer_key_down[code] = pressed;

    if (pressed) {
      cardputer_set_repeat_key(code);
    } else {
      cardputer_clear_repeat_key(code);
    }

    cardputer_handle_key(code, pressed);
  }

  if (interrupt_status & (TCA8418_INT_KEY | TCA8418_INT_OVERFLOW)) {
    if (interrupt_status & TCA8418_INT_OVERFLOW) {
      cardputer_reset_local_state();
    }

    if (!tca8418_write_register(
          TCA8418_REG_INT_STAT,
          (uint8_t)(TCA8418_INT_KEY | TCA8418_INT_OVERFLOW)
        )) {

      i2c_ok = false;

      cardputer_handle_i2c_error();
    }
  }

  if (i2c_ok) {
    s_cardputer_i2c_error_count = 0;
  }

  cardputer_update_repeat();
}
