#include "area512_ti_class.h"
#include "area512_ti_builtin_db.h"
#include "area512_ti_define_info.h"
#include "area512_ti_t.h"
#include "area512_ti_t_frame.h"
#include <stdint.h>

void
ti_eval_class(TiContext *context, const pm_class_node_t *class_node) {
  uint16_t name_id;

  if (!ti_convert_constant_id(class_node->name, &name_id)) {
    context->failed = 1;
    return;
  }

  uint16_t define_row =
    ti_calculate_row(context, class_node->base.location.start);

  TiDefineInfo *define_info = ti_set_define_info(name_id, 0, define_row, 1);

  if (!define_info)
    return;

  uint16_t class_t_index = ti_new_t(TI_CLASS_CLASS, TI_T_FLAG_STATIC, 0);

  if (class_t_index == 0 || !ti_set_value_t(name_id, class_t_index))
    context->failed = 1;
}
