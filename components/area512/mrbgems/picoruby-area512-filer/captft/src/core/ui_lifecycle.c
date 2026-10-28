// Enter/leave the filer's full-screen UI: owns the row sprite while active and
// releases it on teardown.
#if defined(PICORB_VM_MRUBYC)

#include "area512_hal.h"
#include "core/filer.h"
#include "core/terminal/terminal.h"
#include "io-console.h"

void
area512_filer_setup_ui(Filer *filer) {
  area512_gfx_set_window(0, 0, 0, 0);

  if (!filer->row) {
    filer->full_redraw = 1;
    filer->drawn.valid = 0;

    filer->row =
      area512_sprite_new_with_font_size(
        SCREEN_WIDTH,
        ROW_HEIGHT,
        FILER_FONT_SIZE
      );
  }
}

void
area512_filer_teardown_ui(Filer *filer) {
  filer->drawn.valid = 0;

  if (filer->row) {
    area512_sprite_delete(filer->row);
    filer->row = 0;
  }
}

void
draw_current_screen(void *context) {
  Filer *filer = context;

  if (filer->terminal)
    draw_terminal(filer);
  else
    draw_all(filer);
}

int
read_filer_action(Filer *filer) {
  area512_filer_setup_ui(filer);
  clamp_index(filer);

  if (filer->is_window_open) {
    filer->is_window_open = 0;

    area512_start_filer_window_close_animation(draw_current_screen, filer);

    filer->full_redraw = 1;
  }

  // Raw so Ctrl-C/Ctrl-Z reach the filer as bytes instead of becoming signals.
  io_raw_bang(false);

  int action = run_filer_interaction(filer);

  io_cooked_bang();

  switch (action) {
  case ACTION_RUN_RUBY:
  case ACTION_RUN_PYTHON:
  case ACTION_RUN_DIR:
  case ACTION_EDIT:
  case ACTION_EDIT_DOT:
  case ACTION_VIEW_MARKDOWN:
  case ACTION_IRB:
  case ACTION_PYTHON_REPL:
  case ACTION_RECEIVE_FILES:
  case ACTION_INSTALL_APP_IMAGE:
  case ACTION_UNINSTALL_APP:
    area512_filer_setup_ui(filer);
    filer->is_window_open = 1;

    if (filer->terminal) {
      area512_filer_set_gfx_window();
      filer->full_redraw = 0;
    }

    area512_start_filer_window_open_animation(draw_current_screen, filer);
    draw_current_screen(filer);
    area512_filer_set_gfx_window();
    area512_filer_teardown_ui(filer);

    break;

  default:
    area512_gfx_set_window(0, 0, 0, 0);
    break;
  }

  return action;
}

void
area512_filer_prepare_next_app(Filer *filer) {
  // The previous app may have drawn outside the window (Display.full_screen),
  // so repaint the filer around the window before the next app starts in it.
  area512_filer_setup_ui(filer);
  filer->is_window_open = 1;

  draw_current_screen(filer);
  area512_filer_set_gfx_window();
  area512_filer_teardown_ui(filer);
}

#endif
