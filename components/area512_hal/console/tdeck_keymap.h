#pragma once

#include <stdint.h>

static constexpr int KEYBOARD_COLUMN_COUNT = 5;
static constexpr int KEYBOARD_ROW_COUNT = 7;

bool tdeck_keymap_contains_key(int column, int row);
bool tdeck_keymap_lookup_output(
  int column,
  int row,
  bool is_alt_pressed,
  bool is_shift_pressed,
  bool is_symbol_pressed,
  uint8_t *output
);
