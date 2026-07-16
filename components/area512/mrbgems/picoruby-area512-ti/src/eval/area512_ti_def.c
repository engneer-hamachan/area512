#include "area512_ti_def.h"
#include "area512_ti_define_info.h"
#include "area512_ti_eval_expression.h"
#include "area512_ti_t.h"
#include "area512_ti_t_frame.h"
#include <stdint.h>

static void
set_define_args(
  TiContext *context,
  TiDefineInfo *define_info,
  const pm_parameters_node_t *parameters
) {
  if (!define_info || !parameters)
    return;

  size_t required_count = parameters->requireds.size;
  if (required_count > TI_DEFINE_ARG_CAPACITY)
    required_count = TI_DEFINE_ARG_CAPACITY;

  define_info->define_arg_count = (uint8_t)required_count;

  for (size_t index = 0; index < required_count; index++) {
    const pm_node_t *parameter = parameters->requireds.nodes[index];

    if (PM_NODE_TYPE(parameter) != PM_REQUIRED_PARAMETER_NODE)
      continue;

    pm_constant_id_t constant_id =
      ((const pm_required_parameter_node_t *)parameter)->name;
    uint16_t name_id;

    if (!ti_convert_constant_id(constant_id, &name_id)) {
      context->failed = 1;
      return;
    }

    define_info->define_arg_name_ids[index] = name_id;
  }
}

void
ti_eval_def(TiContext *context, const pm_def_node_t *def_node) {
  uint16_t name_id;

  if (!ti_convert_constant_id(def_node->name, &name_id)) {
    context->failed = 1;
    return;
  }

  uint16_t define_row =
    ti_calculate_row(context, def_node->base.location.start);
  TiDefineInfo *define_info = ti_set_define_info(
    name_id,
    context->current_class_name_id,
    define_row,
    0
  );

  if (!define_info)
    return;

  set_define_args(context, define_info, def_node->parameters);

  uint16_t outer_return_t_index = context->return_t_index;
  context->return_t_index = 0;

  uint16_t body_t_index = ti_eval_expression(context, def_node->body, 0);
  uint16_t return_t_index =
    ti_make_union(context->return_t_index, body_t_index);

  context->return_t_index = outer_return_t_index;

  if (return_t_index == 0)
    return;

  define_info->return_t_index = return_t_index;
  if (!ti_set_value_t(name_id, return_t_index))
    context->failed = 1;
}
