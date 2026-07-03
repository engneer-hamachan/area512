#ifndef EDIT_TEXT_UTF8_H
#define EDIT_TEXT_UTF8_H

#include <stdint.h>

int vim_utf8_byte_length(uint8_t byte);
int vim_cell_width(uint8_t lead_byte);
int
vim_character_byte_length(const char *text, int byte_length, int byte_offset);
int vim_previous_character_byte_offset(const char *text, int byte_offset);
int vim_display_width(const char *text, int byte_length);
int vim_byte_to_column(const char *text, int byte_length, int byte_position);
int vim_column_to_byte(const char *text, int byte_length, int column);

#endif
