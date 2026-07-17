#include "area512_ti_bind.h"
#include "area512_ti_eval.h"
#include "area512_ti_t_frame.h"
#include <stdint.h>

uint16_t
ti_bind_scalar_assignment(
  TiContext *context,
  pm_constant_id_t constant_id,
  const pm_node_t *value,
  int depth
) {

  uint16_t name_id;

  if (!ti_convert_constant_id(constant_id, &name_id)) {
    context->failed = 1;
    return 0;
  }

  uint16_t t_node_index = ti_eval_expression(context, value, depth + 1);

  if (t_node_index != 0 && !ti_set_value_t(name_id, t_node_index))
    context->failed = 1;

  return t_node_index;
}
