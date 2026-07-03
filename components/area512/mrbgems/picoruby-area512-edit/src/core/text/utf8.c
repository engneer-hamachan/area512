#include "core/text/utf8.h"

int
vim_utf8_byte_length(uint8_t byte) {
  if (byte < 0x80)
    return 1;
  if (byte < 0xE0)
    return 2;
  if (byte < 0xF0)
    return 3;
  return 4;
}

int
vim_cell_width(uint8_t lead_byte) {
  return vim_utf8_byte_length(lead_byte) > 1 ? 2 : 1;
}

int
vim_character_byte_length(const char *text, int byte_length, int byte_offset) {
  if (byte_offset < 0 || byte_offset >= byte_length)
    return 0;
  return vim_utf8_byte_length((uint8_t)text[byte_offset]);
}

int
vim_previous_character_byte_offset(const char *text, int byte_offset) {
  if (byte_offset <= 0)
    return 0;

  int previous_byte_offset = byte_offset - 1;

  while (previous_byte_offset > 0 &&
         ((uint8_t)text[previous_byte_offset] & 0xC0) == 0x80)
    previous_byte_offset--;

  return previous_byte_offset;
}

int
vim_display_width(const char *text, int byte_length) {
  int width = 0, byte_offset = 0;
  while (byte_offset < byte_length) {
    width += vim_cell_width((uint8_t)text[byte_offset]);
    byte_offset += vim_utf8_byte_length((uint8_t)text[byte_offset]);
  }
  return width;
}

int
vim_byte_to_column(const char *text, int byte_length, int byte_position) {
  int column = 0, byte_offset = 0;
  while (byte_offset < byte_position && byte_offset < byte_length) {
    column += vim_cell_width((uint8_t)text[byte_offset]);
    byte_offset += vim_utf8_byte_length((uint8_t)text[byte_offset]);
  }
  return column;
}

int
vim_column_to_byte(const char *text, int byte_length, int column) {
  int current_column = 0, byte_offset = 0;
  while (current_column < column && byte_offset < byte_length) {
    current_column += vim_cell_width((uint8_t)text[byte_offset]);
    byte_offset += vim_utf8_byte_length((uint8_t)text[byte_offset]);
  }
  return byte_offset;
}
