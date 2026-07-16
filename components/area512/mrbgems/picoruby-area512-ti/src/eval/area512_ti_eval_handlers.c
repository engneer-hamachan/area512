#include "area512_ti_eval_handlers.h"
#include "area512_ti_builtin.h"
#include "area512_ti_define_info.h"
#include "area512_ti_t.h"
#include "area512_ti_t_frame.h"

uint16_t
ti_handle_identifier(pm_constant_id_t constant_id) {
  uint16_t name_id;

  if (!ti_convert_constant_id(constant_id, &name_id))
    return 0;

  return ti_get_value_t(name_id);
}

uint16_t
ti_handle_const_evaluation(
  TiContext *context,
  const pm_constant_read_node_t *constant_read
) {
  const pm_constant_t *constant = ti_get_constant(context, constant_read->name);

  if (constant) {
    uint8_t class_id =
      ti_get_builtin_class_id(constant->start, constant->length);

    if (class_id != TI_CLASS_NONE) {
      return ti_new_t(class_id, TI_T_FLAG_STATIC, 0, 0);
    }
  }

  uint16_t name_id;
  if (!ti_convert_constant_id(constant_read->name, &name_id))
    return 0;

  uint8_t user_class_id = ti_get_defined_class_id(name_id);
  if (user_class_id != TI_CLASS_NONE) {
    return ti_new_t(
      user_class_id,
      TI_T_FLAG_DEFINED_CLASS | TI_T_FLAG_STATIC,
      0,
      0
    );
  }

  return ti_get_value_t(name_id);
}
