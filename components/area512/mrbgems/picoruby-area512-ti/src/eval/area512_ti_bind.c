#include "area512_ti_bind.h"
#include "area512_ti_eval_expression.h"
#include "area512_ti_t_frame.h"
#include <stdint.h>

void
ti_bind_scalar_assignment(
  TiContext *context,
  pm_constant_id_t constant_id,
  const pm_node_t *value
) {

  uint16_t name_id;

  if (!ti_convert_constant_id(constant_id, &name_id)) {
    context->failed = 1;
    return;
  }

  uint16_t t_index = ti_eval_expression(context, value, 0);

  if (t_index != 0 && !ti_set_value_t(name_id, t_index))
    context->failed = 1;
}
