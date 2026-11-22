#if defined(PICORB_VM_MRUBYC)

#include "core/terminal/terminal.h"
#include "area512_hal.h"
#include "core/draw/draw.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void
draw_all_on_animation_frame(void *context) {
  draw_all(context);
}

void
open_terminal_session(Filer *filer) {
  Terminal *terminal = (Terminal *)malloc(sizeof(Terminal));

  if (terminal == NULL)
    return;

  memset(terminal, 0, sizeof(Terminal));

  terminal->visible_output_row_count = AREA512_WINDOW_HEIGHT / ROW_HEIGHT - 1;

  if (terminal->visible_output_row_count > TERMINAL_OUTPUT_ROW_CAPACITY)
    terminal->visible_output_row_count = TERMINAL_OUTPUT_ROW_CAPACITY;

  terminal->line_byte_capacity =
    (AREA512_WINDOW_WIDTH - TERMINAL_CONTENT_LEFT_X * 2) / FILER_CHAR_WIDTH;

  terminal->entry_index_to_restore = filer->index;
  terminal->completion_match_index = -1;

  filer->message[0] = 0;
  filer->terminal = terminal;

  open_soft_keyboard_unless_bluetooth_keyboard_connected(filer);

  area512_start_filer_window_open_animation(draw_all_on_animation_frame, filer);

  filer->full_redraw = 1;
}

int
run_terminal_session(Filer *filer) {
  Terminal *terminal = filer->terminal;

  int filer_action;
  char command_echo_text[LINE_MAX];

  open_soft_keyboard_unless_bluetooth_keyboard_connected(filer);

  if (filer->message[0]) {
    append_output_text(terminal, filer->message);

    filer->message[0] = 0;
  }

  for (;;) {
    read_terminal_input_line(filer);

    if (terminal->input_byte_count == 0)
      continue;

    snprintf(
      command_echo_text,
      sizeof command_echo_text,
      "%s $ %s",
      filer->current_directory,
      terminal->input_line
    );

    append_output_text(terminal, command_echo_text);
    append_history_line(terminal, terminal->input_line);

    filer_action = execute_terminal_command(filer);

    if (filer_action == ACTION_CHANGE_DIR)
      terminal->entry_index_to_restore = 0;

    if (filer_action == TERMINAL_ACTION_EXIT)
      return TERMINAL_ACTION_EXIT;

    if (filer_action != ACTION_NONE)
      return filer_action;
  }
}

void
close_terminal_session(Filer *filer) {
  close_soft_keyboard(filer);
  clear_filer_touch_events(filer);
  area512_gfx_set_window(0, 0, 0, 0);
  filer->action_target_path[0] = 0;
  filer->index = filer->terminal->entry_index_to_restore;

  clamp_index(filer);

  free(filer->terminal);

  filer->terminal = NULL;
  filer->closing_window_left = find_window_left(filer);
  filer->closing_window_top = find_window_top(filer);

  area512_start_filer_window_close_animation(draw_current_screen, filer);

  filer->full_redraw = 1;
}

#endif
