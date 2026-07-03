// Enter/leave the filer's full-screen UI: owns the row sprite while active and
// releases it on teardown.
#if defined(PICORB_VM_MRUBYC)

#include "core/filer.h"
#include "area512_hal.h"

void
area512_filer_setup_ui(Filer *filer) {
  area512_gfx_fill_screen(COLOR_BACKGROUND);

  if (!filer->row) {
    filer->row =
      area512_sprite_new_with_font_size(
        filer->width,
        ROW_HEIGHT,
        FILER_FONT_SIZE
      );
  }

  filer->full_redraw = 1;
}

void
area512_filer_teardown_ui(Filer *filer) {
  if (filer->row) {
    area512_sprite_delete(filer->row);
    filer->row = 0;
  }
}

#endif
