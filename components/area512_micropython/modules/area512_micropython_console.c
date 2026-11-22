#include "area512_hal.h"

#include "py/runtime.h"

static mp_obj_t
reset_console(void) {
  area512_console_reset();

  return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(reset_console_callable, reset_console);

static mp_obj_t
wait_console_key_if_output(void) {
  area512_console_wait_key_if_output();

  return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(
  wait_console_key_if_output_callable,
  wait_console_key_if_output
);

static const mp_rom_map_elem_t console_locals_table[] = {
  {MP_ROM_QSTR(MP_QSTR_reset), MP_ROM_PTR(&reset_console_callable)},
  {
    MP_ROM_QSTR(MP_QSTR_wait_key_if_output),
    MP_ROM_PTR(&wait_console_key_if_output_callable),
  },
};
static MP_DEFINE_CONST_DICT(console_locals_dictionary, console_locals_table);

MP_DEFINE_CONST_OBJ_TYPE(
  area512_micropython_console_type,
  MP_QSTR_Console,
  MP_TYPE_FLAG_NONE,
  locals_dict,
  &console_locals_dictionary
);
