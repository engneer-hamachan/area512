#if defined(PICORB_VM_MRUBYC)

#include "core/terminal/terminal.h"

#include <string.h>

static void
insert_character_at_cursor(Terminal *terminal, char character) {
  char *after_cursor_text =
    terminal->input_line + terminal->input_cursor_byte_offset;

  int after_cursor_byte_count =
    terminal->input_byte_count - terminal->input_cursor_byte_offset;

  if (terminal->input_byte_count >= LINE_MAX - 1)
    return;

  memmove(after_cursor_text + 1, after_cursor_text, after_cursor_byte_count);

  terminal->input_line[terminal->input_cursor_byte_offset] = character;
  terminal->input_cursor_byte_offset++;
  terminal->input_byte_count++;
  terminal->input_line[terminal->input_byte_count] = 0;
}

static void
delete_character_before_cursor(Terminal *terminal) {
  char *after_cursor_text =
    terminal->input_line + terminal->input_cursor_byte_offset;

  int after_cursor_byte_count =
    terminal->input_byte_count - terminal->input_cursor_byte_offset;

  if (terminal->input_cursor_byte_offset == 0)
    return;

  memmove(after_cursor_text - 1, after_cursor_text, after_cursor_byte_count);

  terminal->input_cursor_byte_offset--;
  terminal->input_byte_count--;
  terminal->input_line[terminal->input_byte_count] = 0;
}

static void
move_input_cursor(Terminal *terminal, int input_cursor_byte_offset_delta) {
  int input_cursor_byte_offset =
    terminal->input_cursor_byte_offset + input_cursor_byte_offset_delta;

  if (
    input_cursor_byte_offset < 0 ||
    input_cursor_byte_offset > terminal->input_byte_count
  )
    return;

  terminal->input_cursor_byte_offset = input_cursor_byte_offset;
}

static void
clear_input_line(Terminal *terminal) {
  terminal->input_line[0] = 0;
  terminal->input_byte_count = 0;
  terminal->input_cursor_byte_offset = 0;
  terminal->visible_input_byte_offset = 0;
}

static void
append_autosuggestion_to_input_line(Filer *filer) {
  char autosuggestion[LINE_MAX];
  int autosuggestion_byte_offset = 0;

  build_autosuggestion(filer, autosuggestion, sizeof autosuggestion);

  while (autosuggestion[autosuggestion_byte_offset]) {
    insert_character_at_cursor(
      filer->terminal,
      autosuggestion[autosuggestion_byte_offset]
    );

    autosuggestion_byte_offset++;
  }
}

static void
recall_history_line(Terminal *terminal, int history_recall_index_delta) {
  int history_recall_index =
    terminal->history_recall_index + history_recall_index_delta;

  if (
    history_recall_index < 0 ||
    history_recall_index > terminal->history_line_count
  )
    return;

  terminal->history_recall_index = history_recall_index;

  if (history_recall_index == terminal->history_line_count) {
    clear_input_line(terminal);

    return;
  }

  strncpy(
    terminal->input_line,
    terminal->history_lines[history_recall_index],
    LINE_MAX - 1
  );

  terminal->input_line[LINE_MAX - 1] = 0;
  terminal->input_byte_count = (int)strlen(terminal->input_line);
  terminal->input_cursor_byte_offset = terminal->input_byte_count;
}

void
read_terminal_input_line(Filer *filer) {
  Terminal *terminal = filer->terminal;
  int input_key;

  clear_input_line(terminal);

  terminal->history_recall_index = terminal->history_line_count;
  terminal->completion_match_index = -1;

  for (;;) {
    draw_terminal(filer);

    input_key = read_terminal_key();

    if (input_key != '\t')
      terminal->completion_match_index = -1;

    if (input_key == '\r' || input_key == '\n')
      return;

    if (input_key == '\t')
      complete_input_word(filer);
    else if (input_key == TERMINAL_APPEND_AUTOSUGGESTION_KEY)
      append_autosuggestion_to_input_line(filer);
    else if (input_key == '\b' || input_key == 127)
      delete_character_before_cursor(terminal);
    else if (input_key == 27)
      clear_input_line(terminal);
    else if (input_key == KEY_UP)
      recall_history_line(terminal, -1);
    else if (input_key == KEY_DOWN)
      recall_history_line(terminal, 1);
    else if (input_key == KEY_LEFT)
      move_input_cursor(terminal, -1);
    else if (input_key == KEY_RIGHT)
      move_input_cursor(terminal, 1);
    else if (input_key >= ' ' && input_key <= '~')
      insert_character_at_cursor(terminal, (char)input_key);
  }
}

#endif
