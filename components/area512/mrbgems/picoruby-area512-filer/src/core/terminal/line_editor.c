#if defined(PICORB_VM_MRUBYC)

#include "core/terminal/terminal.h"

#include <string.h>

static void
append_input_character(Terminal *terminal, char character) {
  if (terminal->input_byte_count >= LINE_MAX - 1)
    return;

  terminal->input_line[terminal->input_byte_count] = character;
  terminal->input_byte_count++;
  terminal->input_line[terminal->input_byte_count] = 0;
}

static void
remove_last_input_character(Terminal *terminal) {
  if (terminal->input_byte_count == 0)
    return;

  terminal->input_byte_count--;
  terminal->input_line[terminal->input_byte_count] = 0;
}

static void
clear_input_line(Terminal *terminal) {
  terminal->input_line[0] = 0;
  terminal->input_byte_count = 0;
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
    else if (input_key == '\b' || input_key == 127)
      remove_last_input_character(terminal);
    else if (input_key == 27)
      clear_input_line(terminal);
    else if (input_key == KEY_UP)
      recall_history_line(terminal, -1);
    else if (input_key == KEY_DOWN)
      recall_history_line(terminal, 1);
    else if (input_key >= ' ' && input_key <= '~')
      append_input_character(terminal, (char)input_key);
  }
}

#endif
