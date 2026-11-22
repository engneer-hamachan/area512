#if defined(PICORB_VM_MRUBYC)

#include "area512_hal.h"
#include "core/filer.h"

void
open_soft_keyboard_unless_bluetooth_keyboard_connected(Filer *filer) {
  if (
    area512_is_soft_keyboard_open() ||
    area512_bluetooth_is_keyboard_connected()
  )
    return;

  area512_open_soft_keyboard();

  filer->full_redraw = 1;
}

void
close_soft_keyboard(Filer *filer) {
  if (!area512_is_soft_keyboard_open())
    return;

  area512_close_soft_keyboard();

  filer->full_redraw = 1;
}

#endif
