#if defined(PICORB_VM_MRUBYC)

#include "area512_hal.h"
#include "core/draw/draw.h"
#include "core/filer.h"

#define DRAG_DISTANCE_PER_ROW 17

static int
find_tapped_entry_key(Filer *filer, int row_index) {
  int entry_index = filer->top + row_index;

  if (entry_index >= filer->count)
    return KEY_NONE;

  if (entry_index == filer->index)
    return KEY_RIGHT;

  jump_to(filer, row_index);

  return KEY_NONE;
}

static int
find_tapped_key(Filer *filer, int touch_screen_x, int touch_screen_y) {
  if (touch_screen_y >= BAR_Y) {
    if (filer->message[0] && is_action_bar_at(touch_screen_y)) {
      filer->message[0] = 0;

      return KEY_NONE;
    }

    return find_action_bar_key_at(touch_screen_x, touch_screen_y);
  }

  if (touch_screen_y < HEADER_HEIGHT)
    return KEY_NONE;

  return find_tapped_entry_key(
    filer,
    (touch_screen_y - HEADER_HEIGHT) / ROW_HEIGHT
  );
}

static void
move_cursor_by_drag(Filer *filer, int touch_screen_x, int touch_screen_y) {
  if (!filer->is_touch_dragging) {
    filer->is_touch_dragging = 1;
    filer->touch_drag_axis = AREA512_TOUCH_DRAG_AXIS_NONE;
    filer->touch_drag_anchor_y = touch_screen_y;
    filer->touch_drag_start_x = touch_screen_x;
    filer->touch_drag_start_y = touch_screen_y;

    return;
  }

  if (filer->touch_drag_axis == AREA512_TOUCH_DRAG_AXIS_NONE)
    filer->touch_drag_axis = area512_find_touch_drag_axis(
      touch_screen_x - filer->touch_drag_start_x,
      touch_screen_y - filer->touch_drag_start_y
    );

  if (filer->touch_drag_axis != AREA512_TOUCH_DRAG_AXIS_VERTICAL)
    return;

  int row_delta =
    (filer->touch_drag_anchor_y - touch_screen_y) / DRAG_DISTANCE_PER_ROW;

  if (row_delta == 0)
    return;

  filer->touch_drag_anchor_y -= row_delta * DRAG_DISTANCE_PER_ROW;

  move_cursor(filer, row_delta);
}

static int
find_swipe_key(Filer *filer, int touch_screen_x, int touch_screen_y) {
  if (!filer->is_touch_dragging)
    return KEY_NONE;

  filer->is_touch_dragging = 0;

  switch (
    area512_find_touch_swipe_direction(
      filer->touch_drag_axis,
      touch_screen_x - filer->touch_drag_start_x,
      touch_screen_y - filer->touch_drag_start_y
    )
  ) {

  case AREA512_TOUCH_SWIPE_RIGHT:
    return KEY_LEFT;

  case AREA512_TOUCH_SWIPE_LEFT:
    return KEY_RIGHT;

  default:
    return KEY_NONE;
  }
}

// Drops the queued events together with the drag they belonged to, so the
// next DRAG starts a new drag instead of continuing one whose RELEASE was
// dropped.
void
clear_filer_touch_events(Filer *filer) {
  area512_clear_touch_events();
  filer->is_touch_dragging = 0;
}

int
read_filer_touch_key(Filer *filer) {
  int kind;
  int touch_screen_x;
  int touch_screen_y;
  int64_t event_uptime_ms;

  while (
    area512_read_touch_event(
      &kind,
      &touch_screen_x,
      &touch_screen_y,
      &event_uptime_ms
    )
  ) {

    int key;

    if (kind == AREA512_TOUCH_DRAG) {
      move_cursor_by_drag(filer, touch_screen_x, touch_screen_y);

      continue;
    }

    if (kind == AREA512_TOUCH_TAP)
      key = find_tapped_key(filer, touch_screen_x, touch_screen_y);
    else
      key = find_swipe_key(filer, touch_screen_x, touch_screen_y);

    if (key != KEY_NONE)
      return key;
  }

  return KEY_NONE;
}

#endif
