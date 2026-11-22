#include "soft_keyboard.h"

#include "area512_hal.h"
#include "console_internal.h"

#include <atomic>
#include <pthread.h>
#include <stdint.h>

static constexpr int KEY_ROW_COUNT = 4;
static constexpr int KEY_ROW_SLOT_COUNT = 11;
static constexpr int KEY_UNIT_WIDTH = 32;
static constexpr int KEY_HEIGHT = 24;

namespace {
enum KeyAction {
  KEY_ACTION_OUTPUT,
  KEY_ACTION_SHIFT,
  KEY_ACTION_SWITCH_LAYER,
  KEY_ACTION_CTRL,
};

struct SoftKeyboardKey {
  const char *label;
  const char *output;
  int unit_count;
  KeyAction action;
  bool is_pressed_color_swapped;
};
}

#define CHARACTER_KEY(character) \
  {character, character, 1, KEY_ACTION_OUTPUT, false}
// a r e 5 1 2 spell AREA512 across the keyboard.
#define PRESSED_COLOR_SWAPPED_CHARACTER_KEY(character) \
  {character, character, 1, KEY_ACTION_OUTPUT, true}
#define BACKSPACE_KEY {"BS", "\b", 1, KEY_ACTION_OUTPUT, false}
#define ENTER_KEY {"Enter", "\r", 1, KEY_ACTION_OUTPUT, false}
#define SHIFT_KEY {"Shift", "", 1, KEY_ACTION_SHIFT, false}
#define TAB_KEY {"Tab", "\t", 1, KEY_ACTION_OUTPUT, false}
#define ESCAPE_KEY {"Esc", "\x1b", 1, KEY_ACTION_OUTPUT, false}
#define CTRL_KEY {"Ctrl", "", 1, KEY_ACTION_CTRL, false}
#define SPACE_KEY {"Space", " ", 3, KEY_ACTION_OUTPUT, false}
#define LEFT_KEY {"←", "\x1b[D", 1, KEY_ACTION_OUTPUT, false}
#define UP_KEY {"↑", "\x1b[A", 1, KEY_ACTION_OUTPUT, false}
#define DOWN_KEY {"↓", "\x1b[B", 1, KEY_ACTION_OUTPUT, false}
#define RIGHT_KEY {"→", "\x1b[C", 1, KEY_ACTION_OUTPUT, false}
#define END_OF_ROW {nullptr, nullptr, 0, KEY_ACTION_OUTPUT, false}
#define LETTER_LAYER_FOURTH_ROW \
  { \
    {"123", "", 1, KEY_ACTION_SWITCH_LAYER, false}, ESCAPE_KEY, CTRL_KEY, \
    SPACE_KEY, LEFT_KEY, UP_KEY, DOWN_KEY, RIGHT_KEY, END_OF_ROW, \
  }
#define SYMBOL_LAYER_FOURTH_ROW \
  { \
    {"abc", "", 1, KEY_ACTION_SWITCH_LAYER, false}, ESCAPE_KEY, CTRL_KEY, \
    SPACE_KEY, LEFT_KEY, UP_KEY, DOWN_KEY, RIGHT_KEY, END_OF_ROW, \
  }

static const SoftKeyboardKey LETTER_LAYER[KEY_ROW_COUNT][KEY_ROW_SLOT_COUNT] = {
  {
    CHARACTER_KEY("q"), CHARACTER_KEY("w"),
    PRESSED_COLOR_SWAPPED_CHARACTER_KEY("e"),
    PRESSED_COLOR_SWAPPED_CHARACTER_KEY("r"),
    CHARACTER_KEY("t"), CHARACTER_KEY("y"),
    CHARACTER_KEY("u"), CHARACTER_KEY("i"), CHARACTER_KEY("o"),
    CHARACTER_KEY("p"), END_OF_ROW,
  },
  {
    PRESSED_COLOR_SWAPPED_CHARACTER_KEY("a"),
    CHARACTER_KEY("s"), CHARACTER_KEY("d"),
    CHARACTER_KEY("f"), CHARACTER_KEY("g"), CHARACTER_KEY("h"),
    CHARACTER_KEY("j"), CHARACTER_KEY("k"), CHARACTER_KEY("l"),
    BACKSPACE_KEY, END_OF_ROW,
  },
  {
    SHIFT_KEY, CHARACTER_KEY("z"), CHARACTER_KEY("x"),
    CHARACTER_KEY("c"), CHARACTER_KEY("v"), CHARACTER_KEY("b"),
    CHARACTER_KEY("n"), CHARACTER_KEY("m"), CHARACTER_KEY("."),
    ENTER_KEY, END_OF_ROW,
  },
  LETTER_LAYER_FOURTH_ROW,
};

static const SoftKeyboardKey SHIFTED_LETTER_LAYER[KEY_ROW_COUNT][KEY_ROW_SLOT_COUNT] = {
  {
    CHARACTER_KEY("Q"), CHARACTER_KEY("W"),
    PRESSED_COLOR_SWAPPED_CHARACTER_KEY("E"),
    PRESSED_COLOR_SWAPPED_CHARACTER_KEY("R"),
    CHARACTER_KEY("T"), CHARACTER_KEY("Y"),
    CHARACTER_KEY("U"), CHARACTER_KEY("I"), CHARACTER_KEY("O"),
    CHARACTER_KEY("P"), END_OF_ROW,
  },
  {
    PRESSED_COLOR_SWAPPED_CHARACTER_KEY("A"),
    CHARACTER_KEY("S"), CHARACTER_KEY("D"),
    CHARACTER_KEY("F"), CHARACTER_KEY("G"), CHARACTER_KEY("H"),
    CHARACTER_KEY("J"), CHARACTER_KEY("K"), CHARACTER_KEY("L"),
    BACKSPACE_KEY, END_OF_ROW,
  },
  {
    SHIFT_KEY, CHARACTER_KEY("Z"), CHARACTER_KEY("X"),
    CHARACTER_KEY("C"), CHARACTER_KEY("V"), CHARACTER_KEY("B"),
    CHARACTER_KEY("N"), CHARACTER_KEY("M"), CHARACTER_KEY(","),
    ENTER_KEY, END_OF_ROW,
  },
  LETTER_LAYER_FOURTH_ROW,
};

static const SoftKeyboardKey SYMBOL_LAYER[KEY_ROW_COUNT][KEY_ROW_SLOT_COUNT] = {
  {
    PRESSED_COLOR_SWAPPED_CHARACTER_KEY("1"),
    PRESSED_COLOR_SWAPPED_CHARACTER_KEY("2"),
    CHARACTER_KEY("3"), CHARACTER_KEY("4"),
    PRESSED_COLOR_SWAPPED_CHARACTER_KEY("5"), CHARACTER_KEY("6"),
    CHARACTER_KEY("7"), CHARACTER_KEY("8"), CHARACTER_KEY("9"),
    CHARACTER_KEY("0"), END_OF_ROW,
  },
  {
    CHARACTER_KEY("-"), CHARACTER_KEY("/"), CHARACTER_KEY(":"),
    CHARACTER_KEY(";"), CHARACTER_KEY("("), CHARACTER_KEY(")"),
    CHARACTER_KEY("\""), CHARACTER_KEY("'"), CHARACTER_KEY("="),
    BACKSPACE_KEY, END_OF_ROW,
  },
  {
    SHIFT_KEY, CHARACTER_KEY("."), CHARACTER_KEY(","),
    CHARACTER_KEY("?"), CHARACTER_KEY("!"), CHARACTER_KEY("_"),
    CHARACTER_KEY("@"), CHARACTER_KEY("#"), TAB_KEY,
    ENTER_KEY, END_OF_ROW,
  },
  SYMBOL_LAYER_FOURTH_ROW,
};

static const SoftKeyboardKey SHIFTED_SYMBOL_LAYER[KEY_ROW_COUNT][KEY_ROW_SLOT_COUNT] = {
  {
    CHARACTER_KEY("["), CHARACTER_KEY("]"), CHARACTER_KEY("{"),
    CHARACTER_KEY("}"), CHARACTER_KEY("<"), CHARACTER_KEY(">"),
    CHARACTER_KEY("+"), CHARACTER_KEY("*"), CHARACTER_KEY("%"),
    CHARACTER_KEY("^"), END_OF_ROW,
  },
  {
    CHARACTER_KEY("\\"), CHARACTER_KEY("|"), CHARACTER_KEY("~"),
    CHARACTER_KEY("`"), CHARACTER_KEY("$"), CHARACTER_KEY("&"),
    CHARACTER_KEY("\""), CHARACTER_KEY("'"), CHARACTER_KEY("="),
    BACKSPACE_KEY, END_OF_ROW,
  },
  {
    SHIFT_KEY, CHARACTER_KEY("."), CHARACTER_KEY(","),
    CHARACTER_KEY("?"), CHARACTER_KEY("!"), CHARACTER_KEY("_"),
    CHARACTER_KEY("@"), CHARACTER_KEY("#"), TAB_KEY,
    ENTER_KEY, END_OF_ROW,
  },
  SYMBOL_LAYER_FOURTH_ROW,
};

static std::atomic<bool> s_is_open(false);
static pthread_mutex_t s_soft_keyboard_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool s_is_symbol_layer = false;
static bool s_is_shift_latched = false;
static bool s_is_ctrl_latched = false;
static int s_pressed_row_index = -1;
static int s_pressed_key_index = -1;

static const SoftKeyboardKey *
find_row_keys(int row_index) {
  if (s_is_symbol_layer)
    return s_is_shift_latched ? SHIFTED_SYMBOL_LAYER[row_index]
                              : SYMBOL_LAYER[row_index];

  return s_is_shift_latched ? SHIFTED_LETTER_LAYER[row_index]
                            : LETTER_LAYER[row_index];
}

static int
compute_key_screen_left(const SoftKeyboardKey *row_keys, int key_index) {
  int unit_offset = 0;

  for (int i = 0; i < key_index; i++)
    unit_offset += row_keys[i].unit_count;

  return unit_offset * KEY_UNIT_WIDTH;
}

static int
find_key_index_at(const SoftKeyboardKey *row_keys, int touch_screen_x) {
  int key_screen_right = 0;

  for (int i = 0; row_keys[i].label != nullptr; i++) {
    key_screen_right += row_keys[i].unit_count * KEY_UNIT_WIDTH;

    if (touch_screen_x < key_screen_right)
      return i;
  }

  return -1;
}

static bool
is_key_latched(const SoftKeyboardKey *key) {
  switch (key->action) {
  case KEY_ACTION_SHIFT:
    return s_is_shift_latched;

  case KEY_ACTION_CTRL:
    return s_is_ctrl_latched;

  default:
    return false;
  }
}

static void
draw_key(int row_index, int key_index, bool is_pressed) {
  const SoftKeyboardKey *row_keys = find_row_keys(row_index);
  const SoftKeyboardKey *key = &row_keys[key_index];

  area512_gfx_draw_soft_keyboard_key(
    compute_key_screen_left(row_keys, key_index),
    AREA512_SOFT_KEYBOARD_TOP + row_index * KEY_HEIGHT,
    key->unit_count * KEY_UNIT_WIDTH,
    KEY_HEIGHT,
    key->label,
    (is_pressed || is_key_latched(key)) != key->is_pressed_color_swapped
  );
}

static void
draw_all_keys(void) {
  for (int row_index = 0; row_index < KEY_ROW_COUNT; row_index++) {
    const SoftKeyboardKey *row_keys = find_row_keys(row_index);

    for (int i = 0; row_keys[i].label != nullptr; i++)
      draw_key(row_index, i, false);
  }
}

static void
push_key_output(const SoftKeyboardKey *key) {
  if (s_is_ctrl_latched && key->output[0] != '\0' && key->output[1] == '\0') {
    area512_console_stdin_push((uint8_t)(key->output[0] & 0x1F));

    return;
  }

  area512_console_stdin_push_bytes(key->output);
}

void
area512_press_soft_keyboard_key(int touch_screen_x, int touch_screen_y) {
  pthread_mutex_lock(&s_soft_keyboard_mutex);

  int row_index = (touch_screen_y - AREA512_SOFT_KEYBOARD_TOP) / KEY_HEIGHT;

  if (!s_is_open || row_index < 0 || row_index >= KEY_ROW_COUNT) {
    pthread_mutex_unlock(&s_soft_keyboard_mutex);

    return;
  }

  const SoftKeyboardKey *row_keys = find_row_keys(row_index);
  int key_index = find_key_index_at(row_keys, touch_screen_x);

  if (key_index < 0) {
    pthread_mutex_unlock(&s_soft_keyboard_mutex);

    return;
  }

  const SoftKeyboardKey *key = &row_keys[key_index];

  switch (key->action) {
  case KEY_ACTION_OUTPUT:
    push_key_output(key);

    s_pressed_row_index = row_index;
    s_pressed_key_index = key_index;

    draw_key(row_index, key_index, true);

    break;

  case KEY_ACTION_SHIFT:
    s_is_shift_latched = !s_is_shift_latched;

    draw_all_keys();

    break;

  case KEY_ACTION_SWITCH_LAYER:
    s_is_symbol_layer = !s_is_symbol_layer;
    s_is_shift_latched = false;

    draw_all_keys();

    break;

  case KEY_ACTION_CTRL:
    s_is_ctrl_latched = !s_is_ctrl_latched;

    draw_key(row_index, key_index, false);

    break;
  }

  pthread_mutex_unlock(&s_soft_keyboard_mutex);
}

void
area512_release_soft_keyboard_key(void) {
  pthread_mutex_lock(&s_soft_keyboard_mutex);

  if (s_is_open && s_pressed_row_index >= 0) {
    if (s_is_shift_latched || s_is_ctrl_latched) {
      s_is_shift_latched = false;
      s_is_ctrl_latched = false;

      draw_all_keys();

    } else {
      draw_key(
        s_pressed_row_index,
        s_pressed_key_index,
        false
      );
    }
  }

  s_pressed_row_index = -1;
  s_pressed_key_index = -1;

  pthread_mutex_unlock(&s_soft_keyboard_mutex);
}

extern "C" void
area512_open_soft_keyboard(void) {
  pthread_mutex_lock(&s_soft_keyboard_mutex);

  bool was_closed = !s_is_open;

  if (was_closed) {
    s_is_symbol_layer = false;
    s_is_shift_latched = false;
    s_is_ctrl_latched = false;
    s_pressed_row_index = -1;
    s_pressed_key_index = -1;
    s_is_open = true;

    area512_gfx_clear_soft_keyboard_area();
    draw_all_keys();
  }

  pthread_mutex_unlock(&s_soft_keyboard_mutex);

  // The window moved up above the keyboard.
  if (was_closed)
    area512_gfx_draw_app_screen();
}

extern "C" void
area512_close_soft_keyboard(void) {
  pthread_mutex_lock(&s_soft_keyboard_mutex);

  bool was_open = s_is_open;

  if (was_open) {
    s_is_open = false;

    area512_gfx_clear_soft_keyboard_area();
  }

  pthread_mutex_unlock(&s_soft_keyboard_mutex);

  // The window moved back down.
  if (was_open)
    area512_gfx_draw_app_screen();
}

extern "C" bool
area512_is_soft_keyboard_open(void) {
  return s_is_open;
}
