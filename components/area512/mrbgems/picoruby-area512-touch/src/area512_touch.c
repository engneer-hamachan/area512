#include <stdint.h>

#if defined(PICORB_VM_MRUBYC)

#include "area512_hal.h"
#include <mrubyc.h>

static const char *
find_touch_kind_name(int kind) {
  switch (kind) {
  case AREA512_TOUCH_TAP:
    return "TAP";

  case AREA512_TOUCH_DRAG:
    return "DRAG";

  default:
    return "RELEASE";
  }
}

static void
c_touch_read(mrbc_vm *virtual_machine, mrbc_value *v, int argument_count) {
  (void)argument_count;

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
  ) {

    SET_NIL_RETURN();

    return;
  }

  mrbc_value event = mrbc_array_new(virtual_machine, 3);

  mrbc_value kind_name =
    mrbc_string_new_cstr(virtual_machine, find_touch_kind_name(kind));

  mrbc_value window_x =
    mrbc_integer_value(touch_screen_x - area512_gfx_window_left());
  mrbc_value window_y =
    mrbc_integer_value(touch_screen_y - area512_gfx_window_top());

  mrbc_array_push(&event, &kind_name);
  mrbc_array_push(&event, &window_x);
  mrbc_array_push(&event, &window_y);

  SET_RETURN(event);
}

static void
c_touch_open_keyboard(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int argument_count
) {

  (void)virtual_machine;
  (void)argument_count;

  area512_open_soft_keyboard();

  SET_NIL_RETURN();
}

static void
c_touch_close_keyboard(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int argument_count
) {

  (void)virtual_machine;
  (void)argument_count;

  area512_close_soft_keyboard();

  SET_NIL_RETURN();
}

static void
c_touch_bluetooth_keyboard_connected(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int argument_count
) {

  (void)virtual_machine;
  (void)argument_count;

  SET_BOOL_RETURN(area512_bluetooth_is_keyboard_connected());
}

void
mrbc_area512_touch_init(mrbc_vm *virtual_machine) {
  mrbc_class *class_Touch =
    mrbc_define_class(virtual_machine, "Touch", mrbc_class_object);

  mrbc_define_method(virtual_machine, class_Touch, "read", c_touch_read);

  mrbc_define_method(
    virtual_machine,
    class_Touch,
    "open_keyboard",
    c_touch_open_keyboard
  );

  mrbc_define_method(
    virtual_machine,
    class_Touch,
    "close_keyboard",
    c_touch_close_keyboard
  );

  mrbc_define_method(
    virtual_machine,
    class_Touch,
    "bluetooth_keyboard_connected?",
    c_touch_bluetooth_keyboard_connected
  );
}

#elif defined(PICORB_VM_MRUBY)

#include <mruby.h>

void
mrb_picoruby_area512_touch_gem_init(mrb_state *state) {
  (void)state;
}

void
mrb_picoruby_area512_touch_gem_final(mrb_state *state) {
  (void)state;
}

#endif
