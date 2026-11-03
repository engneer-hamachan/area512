#include "tdeck_keymap.h"

#include "area512_hal.h"
#include "console_internal.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

static constexpr const char *KEYMAP_PATH = "etc/keymap";
static constexpr int KEYMAP_LINE_BYTE_SIZE = 64;
static constexpr char CONTROL_PREFIX[] = "ctrl-";
static constexpr size_t CONTROL_PREFIX_BYTE_LENGTH = sizeof CONTROL_PREFIX - 1;
static constexpr uint8_t CONTROL_CHARACTER_MASK = 0x1F;

static constexpr int SHIFT_LAYER_BIT = 1 << 0;
static constexpr int SYMBOL_LAYER_BIT = 1 << 1;
static constexpr int ALT_LAYER_BIT = 1 << 2;
static constexpr int LAYER_COUNT = 1 << 3;

static constexpr uint8_t NO_OUTPUT = 0xFF;

static constexpr uint8_t ASCII_ESCAPE = 0x1B;
static constexpr uint8_t ASCII_DELETE = 0x7F;

namespace {
struct NamedCharacter {
  const char *name;
  uint8_t character;
};

struct Key {
  const char *name;
  uint8_t column;
  uint8_t row;
  uint8_t outputs[LAYER_COUNT];
};
} // namespace

static const NamedCharacter NAMED_CHARACTERS[] = {
  {"space", ' '},
  {"enter", '\r'},
  {"tab", '\t'},
  {"esc", ASCII_ESCAPE},
  {"backspace", '\b'},
  {"delete", ASCII_DELETE},
  {"percent", '%'},
  {"ampersand", '&'},
  {"backslash", '\\'},
  {"caret", '^'},
  {"backtick", '`'},
  {"tilde", '~'},
};

// Column/row positions from LilyGO's Keyboard_ESP32C3 firmware.
static Key s_keys[] = {
  {"q", 0, 0, {'q', 'Q', '#', '=', 0x11, 0x11, 0x11, 0x11}},
  {"w", 0, 1, {'w', 'W', '1', 0x11, 0x17, 0x17, 0x17, 0x17}},
  {"e", 1, 0, {'e', 'E', '2', 0x12, 0x05, 0x05, 0x05, 0x05}},
  {"r", 2, 0, {'r', 'R', '3', 0x13, 0x12, 0x12, 0x12, 0x12}},
  {"t", 2, 2, {'t', 'T', '(', '\b', '\t', '\t', '\t', '\t'}},
  {"y", 3, 2, {'y', 'Y', ')', '\t', 0x19, 0x19, 0x19, 0x19}},
  {"u", 3, 0, {'u', 'U', '_', '?', 0x15, 0x15, 0x15, 0x15}},
  {"i", 4, 2, {'i', 'I', '-', '\r', '\t', '\t', '\t', '\t'}},
  {"o", 4, 0, {'o', 'O', '+', 0x0B, 0x0F, 0x0F, 0x0F, 0x0F}},
  {"p", 1, 3, {'p', 'P', '@', ' ', 0x10, 0x10, 0x10, 0x10}},
  {"a", 0, 3, {'a', 'A', '*', '\n', 0x01, 0x01, 0x01, 0x01}},
  {"s", 1, 1, {'s', 'S', '4', 0x14, 0x13, 0x13, 0x13, 0x13}},
  {"d", 1, 2, {'d', 'D', '5', 0x04, 0x04, 0x04, 0x04, 0x04}},
  {"f", 2, 6, {'f', 'F', '6', 0x16, 0x06, 0x06, 0x06, 0x06}},
  {"g", 2, 1, {'g', 'G', '/', '[', 0x07, 0x07, 0x07, 0x07}},
  {"h", 3, 1, {'h', 'H', ':', ']', '\b', '\b', '\b', '\b'}},
  {
    "j", 3, 6,
    {
      'j', 'J', ';', ASCII_ESCAPE,
      ASCII_ESCAPE, ASCII_ESCAPE, ASCII_ESCAPE, ASCII_ESCAPE
    }
  },
  {"k", 4, 6, {'k', 'K', '\'', 0x07, 0x0B, 0x0B, 0x0B, 0x0B}},
  {"l", 4, 1, {'l', 'L', '"', 0x02, 0x0C, 0x0C, 0x0C, 0x0C}},
  {"z", 1, 5, {'z', 'Z', '7', '<', '<', '<', '<', '<'}},
  {"x", 1, 4, {'x', 'X', '8', '>', '>', '>', '>', '>'}},
  {"c", 2, 5, {'c', 'C', '9', 0x03, 0x03, 0x03, 0x03, 0x03}},
  {"v", 2, 4, {'v', 'V', '?', '{', 0x16, 0x16, 0x16, 0x16}},
  {"b", 3, 4, {'b', 'B', '!', '}', 0x02, 0x02, 0x02, 0x02}},
  {"n", 3, 5, {'n', 'N', ',', 0x0E, 0x0E, 0x0E, 0x0E, 0x0E}},
  {"m", 4, 5, {'m', 'M', '.', 0x0E, '\r', '\r', '\r', '\r'}},
  {
    "$", 4, 4,
    {'$', 0x04, NO_OUTPUT, '|', NO_OUTPUT, NO_OUTPUT, NO_OUTPUT, NO_OUTPUT}
  },
  {
    "space", 0, 5,
    {' ', 0x00, NO_OUTPUT, '|', 0x00, NO_OUTPUT, NO_OUTPUT, NO_OUTPUT}
  },
  {
    "mic", 0, 6,
    {'=', 0x1D, '0', 0x10, NO_OUTPUT, NO_OUTPUT, NO_OUTPUT, NO_OUTPUT}
  },
  {"enter", 3, 3, {'\r', '\r', '\r', '\r', '\r', '\r', '\r', '\r'}},
  {"backspace", 4, 3, {'\b', '\b', '\b', '\b', '\b', '\b', '\b', '\b'}},
};

static Key *
find_key_at_position(int column, int row) {
  for (Key &key : s_keys) {
    if (key.column == column && key.row == row)
      return &key;
  }

  return nullptr;
}

static Key *
find_key_by_name(const char *key_name) {
  for (Key &key : s_keys) {
    if (strcmp(key.name, key_name) == 0)
      return &key;
  }

  return nullptr;
}

static int
calculate_layer(bool has_alt, bool has_shift, bool has_symbol) {
  return (has_alt ? ALT_LAYER_BIT : 0) | (has_shift ? SHIFT_LAYER_BIT : 0) |
         (has_symbol ? SYMBOL_LAYER_BIT : 0);
}

bool
tdeck_keymap_contains_key(int column, int row) {
  return find_key_at_position(column, row) != nullptr;
}

bool
tdeck_keymap_lookup_output(
  int column,
  int row,
  bool is_alt_pressed,
  bool is_shift_pressed,
  bool is_symbol_pressed,
  uint8_t *output
) {

  const Key *key = find_key_at_position(column, row);

  if (key == nullptr)
    return false;

  int layer =
    calculate_layer(
      is_alt_pressed,
      is_shift_pressed,
      is_symbol_pressed
    );

  *output = key->outputs[layer];

  return *output != NO_OUTPUT;
}

static bool
parse_left_side(char *left_side, Key **key, int *layer) {
  bool has_alt = false;
  bool has_shift = false;
  bool has_symbol = false;
  char *name = left_side;
  char *dot;

  while ((dot = strchr(name, '.')) != nullptr) {
    *dot = '\0';

    if (strcmp(name, "alt") == 0)
      has_alt = true;
    else if (strcmp(name, "shift") == 0)
      has_shift = true;
    else if (strcmp(name, "sym") == 0)
      has_symbol = true;
    else
      return false;

    name = dot + 1;
  }

  *key = find_key_by_name(name);
  *layer = calculate_layer(has_alt, has_shift, has_symbol);

  return *key != nullptr;
}

static bool
parse_character(const char *character_text, uint8_t *character) {
  if (strlen(character_text) == 1) {
    *character = (uint8_t)character_text[0];

    return true;
  }

  for (const NamedCharacter &named_character : NAMED_CHARACTERS) {
    if (strcmp(character_text, named_character.name) == 0) {
      *character = named_character.character;

      return true;
    }
  }

  return false;
}

static bool
parse_right_side(const char *right_side, uint8_t *output) {
  if (right_side[0] == '\0') {
    *output = NO_OUTPUT;

    return true;
  }

  if (strncmp(right_side, CONTROL_PREFIX, CONTROL_PREFIX_BYTE_LENGTH) != 0)
    return parse_character(right_side, output);

  uint8_t character;

  if (!parse_character(right_side + CONTROL_PREFIX_BYTE_LENGTH, &character))
    return false;

  int uppercase_character = toupper(character);

  if (uppercase_character < '@' || uppercase_character > '_')
    return false;

  *output = (uint8_t)uppercase_character & CONTROL_CHARACTER_MASK;

  return true;
}

static void
assign_keymap_from_line(char *line) {
  char *equal_sign = strchr(line, '=');

  if (equal_sign == nullptr)
    return;

  *equal_sign = '\0';

  Key *key;
  int layer;
  uint8_t output;

  if (
    parse_left_side(line, &key, &layer) &&
    parse_right_side(equal_sign + 1, &output)
  )
    key->outputs[layer] = output;
}

static void
skip_rest_of_line(FILE *file) {
  int character;

  while ((character = fgetc(file)) != EOF && character != '\n') {
  }
}

void
area512_keymap_load(void) {
  char resolved_path[AREA512_PATH_MAX];
  char line[KEYMAP_LINE_BYTE_SIZE];

  if (
    area512_resolve_data_path(
      KEYMAP_PATH,
      resolved_path,
      sizeof resolved_path
    ) < 0
  )
    return;

  FILE *file = fopen(resolved_path, "r");

  if (file == nullptr)
    return;

  while (fgets(line, sizeof line, file) != nullptr) {
    bool is_whole_line_read = strchr(line, '\n') != nullptr;

    line[strcspn(line, "\r\n")] = '\0';

    assign_keymap_from_line(line);

    if (!is_whole_line_read)
      skip_rest_of_line(file);
  }

  fclose(file);
}
