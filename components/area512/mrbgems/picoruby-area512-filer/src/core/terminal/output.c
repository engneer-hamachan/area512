#if defined(PICORB_VM_MRUBYC)

#include "core/terminal/terminal.h"

#include <string.h>

static void
shift_output_lines_up(Terminal *terminal) {
  int output_row_index = 0;

  while (output_row_index < terminal->output_row_count - 1) {
    memcpy(
      terminal->output_lines[output_row_index],
      terminal->output_lines[output_row_index + 1],
      LINE_MAX
    );

    output_row_index++;
  }
}

static void
append_terminal_output_row(
  Terminal *terminal,
  const char *text,
  int text_byte_count
) {

  int output_row_index;

  if (terminal->output_row_count < terminal->visible_output_row_count) {
    output_row_index = terminal->output_row_count;
    terminal->output_row_count++;
  } else {
    shift_output_lines_up(terminal);

    output_row_index = terminal->visible_output_row_count - 1;
  }

  if (text_byte_count > LINE_MAX - 1)
    text_byte_count = LINE_MAX - 1;

  memcpy(terminal->output_lines[output_row_index], text, text_byte_count);

  terminal->output_lines[output_row_index][text_byte_count] = 0;
}

void
append_output_text(Terminal *terminal, const char *text) {
  int text_byte_count = (int)strlen(text);
  int text_byte_offset = 0;
  int output_row_byte_count;

  if (text_byte_count == 0) {
    append_terminal_output_row(terminal, "", 0);

    return;
  }

  while (text_byte_offset < text_byte_count) {
    output_row_byte_count = text_byte_count - text_byte_offset;

    if (output_row_byte_count > terminal->line_byte_capacity)
      output_row_byte_count = terminal->line_byte_capacity;

    append_terminal_output_row(
      terminal,
      text + text_byte_offset,
      output_row_byte_count
    );

    text_byte_offset += output_row_byte_count;
  }
}

void
clear_output_lines(Terminal *terminal) {
  terminal->output_row_count = 0;
}

#endif
