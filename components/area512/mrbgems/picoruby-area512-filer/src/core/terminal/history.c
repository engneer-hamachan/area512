#if defined(PICORB_VM_MRUBYC)

#include "core/terminal/terminal.h"

#include <string.h>

void
append_history_line(Terminal *terminal, const char *input_line) {
  int history_line_index = 0;

  if (terminal->history_line_count == TERMINAL_HISTORY_LINE_CAPACITY) {
    while (history_line_index < TERMINAL_HISTORY_LINE_CAPACITY - 1) {
      memcpy(
        terminal->history_lines[history_line_index],
        terminal->history_lines[history_line_index + 1],
        LINE_MAX
      );

      history_line_index++;
    }

    terminal->history_line_count--;
  }

  strncpy(
    terminal->history_lines[terminal->history_line_count],
    input_line,
    LINE_MAX - 1
  );

  terminal->history_lines[terminal->history_line_count][LINE_MAX - 1] = 0;
  terminal->history_line_count++;
}

#endif
