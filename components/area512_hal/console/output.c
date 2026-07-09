#include "area512_hal.h"
#include "console.h"

#include <stdint.h>
#include <string.h>

#define CONSOLE_ROWS 11
#define CONSOLE_ROW_HEIGHT 12
#define CONSOLE_FONT_SIZE 12
#define CONSOLE_ROW_BYTES 128
#define CONSOLE_TEXT_COLOR 0xCFA45F
#define CONSOLE_BG_COLOR 0x000000

static void *s_row_sprite;
static char s_row_text[CONSOLE_ROWS][CONSOLE_ROW_BYTES];
static int s_row_byte_count[CONSOLE_ROWS];
static int s_row_pixel_width[CONSOLE_ROWS];
static int s_cursor_row_index;
static int s_had_output;

static uint8_t s_character_bytes[4];
static int s_character_filled_byte_count;
static int s_character_total_byte_count;

static void
ensure_row_sprite(void) {
  if (s_row_sprite == NULL) {
    s_row_sprite =
      area512_sprite_new_with_font_size(
        area512_gfx_width(),
        CONSOLE_ROW_HEIGHT,
        CONSOLE_FONT_SIZE
      );
  }
}

static void
clear_row(int row_index) {
  s_row_text[row_index][0] = '\0';
  s_row_byte_count[row_index] = 0;
  s_row_pixel_width[row_index] = 0;
}

static void
push_row_to_screen(int row_index) {
  area512_sprite_fill(s_row_sprite, CONSOLE_BG_COLOR);

  if (s_row_byte_count[row_index] > 0)
    area512_sprite_text(
      s_row_sprite,
      0,
      0,
      s_row_text[row_index],
      CONSOLE_TEXT_COLOR
    );

  area512_sprite_push(s_row_sprite, 0, row_index * CONSOLE_ROW_HEIGHT);
}

static void
push_all_rows_to_screen(void) {
  for (int row_index = 0; row_index < CONSOLE_ROWS; row_index++)
    push_row_to_screen(row_index);
}

static void
drop_oldest_row(void) {
  for (int row_index = 0; row_index < CONSOLE_ROWS - 1; row_index++) {
    memcpy(s_row_text[row_index], s_row_text[row_index + 1], CONSOLE_ROW_BYTES);

    s_row_byte_count[row_index] = s_row_byte_count[row_index + 1];
    s_row_pixel_width[row_index] = s_row_pixel_width[row_index + 1];
  }

  clear_row(CONSOLE_ROWS - 1);
}

static void
advance_to_next_row(void) {
  if (s_cursor_row_index < CONSOLE_ROWS - 1) {
    s_cursor_row_index++;
    clear_row(s_cursor_row_index);
  } else {
    drop_oldest_row();
  }
}

static int
utf8_sequence_byte_count(uint8_t lead_byte) {
  if (lead_byte < 0x80)
    return 1;
  if ((lead_byte & 0xE0) == 0xC0)
    return 2;
  if ((lead_byte & 0xF0) == 0xE0)
    return 3;
  if ((lead_byte & 0xF8) == 0xF0)
    return 4;

  // Stray continuation or invalid lead: consume one byte so we can't stall.
  return 1;
}

static void
append_character_to_current_or_next_row(
  const char *character_bytes,
  int character_byte_count
) {

  char character_string[5];

  memcpy(character_string, character_bytes, character_byte_count);

  character_string[character_byte_count] = '\0';

  int character_pixel_width =
    area512_sprite_text_width(s_row_sprite, character_string);

  if (s_row_pixel_width[s_cursor_row_index] + character_pixel_width > area512_gfx_width() ||
      s_row_byte_count[s_cursor_row_index] + character_byte_count >= CONSOLE_ROW_BYTES - 1) {

    advance_to_next_row();
  }

  memcpy(
    &s_row_text[s_cursor_row_index][s_row_byte_count[s_cursor_row_index]],
    character_bytes,
    character_byte_count
  );

  s_row_byte_count[s_cursor_row_index] += character_byte_count;
  s_row_text[s_cursor_row_index][s_row_byte_count[s_cursor_row_index]] = '\0';
  s_row_pixel_width[s_cursor_row_index] += character_pixel_width;
}

static void
write_character(const char *character_bytes, int character_byte_count) {
  if (character_byte_count == 1) {
    uint8_t byte = (uint8_t)character_bytes[0];

    if (byte == '\n') {
      advance_to_next_row();
      return;
    }

    if (byte < 0x20)
      return;
  }

  append_character_to_current_or_next_row(
    character_bytes,
    character_byte_count
  );
}

static void
write_character_on_utf8_complete(uint8_t byte) {
  if (s_character_total_byte_count == 0) {
    s_character_total_byte_count = utf8_sequence_byte_count(byte);
    s_character_filled_byte_count = 0;
  }

  s_character_bytes[s_character_filled_byte_count++] = byte;

  if (s_character_filled_byte_count >= s_character_total_byte_count) {
    write_character(
      (const char *)s_character_bytes,
      s_character_filled_byte_count
    );

    s_character_total_byte_count = 0;
    s_character_filled_byte_count = 0;
  }
}

void
area512_console_write(const void *buffer, int buffer_byte_count) {
  ensure_row_sprite();

  if (s_row_sprite == NULL)
    return;

  if (buffer_byte_count > 0)
    s_had_output = 1;

  const uint8_t *input_bytes = (const uint8_t *)buffer;

  for (int i = 0; i < buffer_byte_count; i++)
    write_character_on_utf8_complete(input_bytes[i]);

  push_all_rows_to_screen();
}

void
area512_console_reset(void) {
  for (int row_index = 0; row_index < CONSOLE_ROWS; row_index++)
    clear_row(row_index);

  s_cursor_row_index = 0;
  s_had_output = 0;
  s_character_total_byte_count = 0;
  s_character_filled_byte_count = 0;

  area512_gfx_fill_screen(CONSOLE_BG_COLOR);
}

int
area512_console_had_output(void) {
  return s_had_output;
}
