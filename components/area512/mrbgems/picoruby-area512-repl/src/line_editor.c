#if defined(PICORB_VM_MRUBYC)

#include "area512_hal.h"
#include "area512_repl.h"

#include <string.h>

#include <mrubyc.h>

static int
ensure_input_byte_capacity(ReplLine *repl_line, int required_byte_capacity) {
  if (required_byte_capacity <= repl_line->input_byte_capacity)
    return 1;

  int new_byte_capacity = repl_line->input_byte_capacity;

  while (new_byte_capacity < required_byte_capacity)
    new_byte_capacity *= 2;

  char *resized_input_line =
    (char *)mrbc_raw_realloc(repl_line->input_line, new_byte_capacity);

  if (resized_input_line == NULL)
    return 0;

  repl_line->input_line = resized_input_line;
  repl_line->input_byte_capacity = new_byte_capacity;

  return 1;
}

static int
insert_input_character(ReplLine *repl_line, int input_key) {
  if (!ensure_input_byte_capacity(repl_line, repl_line->input_byte_count + 2))
    return 0;

  char *after_cursor_text =
    repl_line->input_line + repl_line->input_cursor_byte_offset;

  int after_cursor_byte_count =
    repl_line->input_byte_count - repl_line->input_cursor_byte_offset;

  memmove(
    after_cursor_text + 1,
    after_cursor_text,
    after_cursor_byte_count + 1
  );

  repl_line->input_line[repl_line->input_cursor_byte_offset] = (char)input_key;

  repl_line->input_byte_count++;
  repl_line->input_cursor_byte_offset++;

  return 1;
}

static void
delete_input_character_before_cursor(ReplLine *repl_line) {
  if (repl_line->input_cursor_byte_offset == 0)
    return;

  char *after_cursor_text =
    repl_line->input_line + repl_line->input_cursor_byte_offset;

  int after_cursor_byte_count =
    repl_line->input_byte_count - repl_line->input_cursor_byte_offset;

  memmove(
    after_cursor_text - 1,
    after_cursor_text,
    after_cursor_byte_count + 1
  );

  repl_line->input_byte_count--;
  repl_line->input_cursor_byte_offset--;
}

static void
move_input_cursor(ReplLine *repl_line, int input_cursor_byte_offset_delta) {
  int new_input_cursor_byte_offset =
    repl_line->input_cursor_byte_offset + input_cursor_byte_offset_delta;

  if (
    new_input_cursor_byte_offset >= 0 &&
    new_input_cursor_byte_offset <= repl_line->input_byte_count
  )
    repl_line->input_cursor_byte_offset = new_input_cursor_byte_offset;
}

static int
read_csi_final_byte(void) {
  int final_byte = area512_console_getch_timeout(40);

  while (
    final_byte >= 0x20 && final_byte <= 0x3f
  )
    final_byte = area512_console_getch_timeout(40);

  return final_byte;
}

static int
read_repl_input_key(void) {
  int first_byte = area512_console_getch_block();

  if (first_byte != 27)
    return first_byte;

  int csi_introducer_byte = area512_console_getch_timeout(40);

  if (csi_introducer_byte != '[')
    return 27;

  int final_byte = read_csi_final_byte();

  if (final_byte == 'D')
    return REPL_KEY_LEFT;

  if (final_byte == 'C')
    return REPL_KEY_RIGHT;

  return 0;
}

int
open_repl_line(ReplLine *repl_line) {
  repl_line->input_line = (char *)mrbc_raw_alloc(16);

  if (repl_line->input_line == NULL)
    return 0;

  repl_line->input_line[0] = '\0';
  repl_line->input_byte_count = 0;
  repl_line->input_byte_capacity = 16;
  repl_line->input_cursor_byte_offset = 0;

  return 1;
}

void
close_repl_line(ReplLine *repl_line) {
  mrbc_raw_free(repl_line->input_line);
}

int
read_repl_input_line(const char *prompt, ReplLine *repl_line) {
  void *row_sprite = create_repl_row_sprite();

  int input_status;

  for (;;) {
    draw_repl_input_line(row_sprite, prompt, repl_line);

    int input_key = read_repl_input_key();

    if (input_key == 27) {
      input_status = REPL_INPUT_ESCAPED;
      break;
    }

    if (input_key == 3) {
      input_status = REPL_INPUT_INTERRUPTED;
      break;
    }

    if (input_key == '\r' || input_key == '\n') {
      input_status = REPL_INPUT_SUBMITTED;
      break;
    }

    if (input_key == 8 || input_key == 127) {
      delete_input_character_before_cursor(repl_line);

    } else if (input_key == REPL_KEY_LEFT) {
      move_input_cursor(repl_line, -1);

    } else if (input_key == REPL_KEY_RIGHT) {
      move_input_cursor(repl_line, 1);

    } else if (input_key >= 0x20 && input_key <= 0x7e) {
      if (!insert_input_character(repl_line, input_key)) {
        input_status = REPL_INPUT_OUT_OF_MEMORY;

        break;
      }
    }
  }

  area512_sprite_delete(row_sprite);

  return input_status;
}

#endif
