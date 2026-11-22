#include "area512_hal.h"

#include "py/runtime.h"

static mp_obj_t
read_touch_event(void) {
  int kind;
  int touch_screen_x;
  int touch_screen_y;
  int64_t event_uptime_ms;

  if (
    !area512_read_touch_event(
      &kind,
      &touch_screen_x,
      &touch_screen_y,
      &event_uptime_ms
    )
  )
    return mp_const_none;

  qstr kind_name = MP_QSTR_RELEASE;

  if (kind == AREA512_TOUCH_TAP)
    kind_name = MP_QSTR_TAP;
  else if (kind == AREA512_TOUCH_DRAG)
    kind_name = MP_QSTR_DRAG;

  mp_obj_t event_items[] = {
    MP_OBJ_NEW_QSTR(kind_name),
    mp_obj_new_int(touch_screen_x - area512_gfx_window_left()),
    mp_obj_new_int(touch_screen_y - area512_gfx_window_top()),
  };

  return mp_obj_new_tuple(3, event_items);
}
static MP_DEFINE_CONST_FUN_OBJ_0(read_touch_event_callable, read_touch_event);

static mp_obj_t
open_touch_keyboard(void) {
  area512_open_soft_keyboard();

  return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_0(
  open_touch_keyboard_callable,
  open_touch_keyboard
);

static mp_obj_t
close_touch_keyboard(void) {
  area512_close_soft_keyboard();

  return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_0(
  close_touch_keyboard_callable,
  close_touch_keyboard
);

static mp_obj_t
is_bluetooth_keyboard_connected(void) {
  return mp_obj_new_bool(area512_bluetooth_is_keyboard_connected());
}

static MP_DEFINE_CONST_FUN_OBJ_0(
  is_bluetooth_keyboard_connected_callable,
  is_bluetooth_keyboard_connected
);

static const mp_rom_map_elem_t touch_locals_table[] = {
  {MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&read_touch_event_callable)},
  {
    MP_ROM_QSTR(MP_QSTR_open_keyboard),
    MP_ROM_PTR(&open_touch_keyboard_callable),
  },
  {
    MP_ROM_QSTR(MP_QSTR_close_keyboard),
    MP_ROM_PTR(&close_touch_keyboard_callable),
  },
  {
    MP_ROM_QSTR(MP_QSTR_bluetooth_keyboard_connected),
    MP_ROM_PTR(&is_bluetooth_keyboard_connected_callable),
  },
};
static MP_DEFINE_CONST_DICT(touch_locals_dictionary, touch_locals_table);

MP_DEFINE_CONST_OBJ_TYPE(
  area512_micropython_touch_type,
  MP_QSTR_Touch,
  MP_TYPE_FLAG_NONE,
  locals_dict,
  &touch_locals_dictionary
);
