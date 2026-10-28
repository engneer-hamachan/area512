#include "area512_hal.h"

#include "py/runtime.h"

// Area512.run_next(path): the filer runs this app as soon as the current one
// returns. An empty path cancels.
static mp_obj_t
run_next_app(mp_obj_t path_object) {
  if (area512_set_next_app(mp_obj_str_get_str(path_object)) != 0)
    mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("path too long"));

  return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(run_next_app_callable, run_next_app);

static const mp_rom_map_elem_t area512_locals_table[] = {
  {MP_ROM_QSTR(MP_QSTR_run_next), MP_ROM_PTR(&run_next_app_callable)},
};
static MP_DEFINE_CONST_DICT(area512_locals_dictionary, area512_locals_table);

MP_DEFINE_CONST_OBJ_TYPE(
  area512_micropython_area512_type,
  MP_QSTR_Area512,
  MP_TYPE_FLAG_NONE,
  locals_dict,
  &area512_locals_dictionary
);
