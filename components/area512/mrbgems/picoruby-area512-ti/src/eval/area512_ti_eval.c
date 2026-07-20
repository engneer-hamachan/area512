#include "area512_ti_eval.h"
#include "area512_ti_arena.h"
#include "area512_ti_bind.h"
#include "area512_ti_builtin.h"
#include "area512_ti_class.h"
#include "area512_ti_def.h"
#include "area512_ti_define_info.h"
#include "area512_ti_eval_handlers.h"
#include "area512_ti_ifunless.h"
#include "area512_ti_method_evaluator.h"
#include "area512_ti_return.h"
#include "area512_ti_square_bracket.h"
#include "area512_ti_t.h"
#include "area512_ti_t_frame.h"

static uint16_t
new_builtin_t(uint8_t class_id) {
  return ti_new_t(class_id, 0, 0);
}

uint16_t
ti_eval_statements(
  TiContext *context,
  const pm_statements_node_t *statements,
  int depth
) {

  if (!statements || depth > TI_EVAL_DEPTH_LIMIT || context->failed)
    return 0;

  if (statements->body.size == 0)
    return new_builtin_t(TI_CLASS_NIL);

  uint16_t last_evaluated_t_node_index = new_builtin_t(TI_CLASS_NIL);

  for (size_t index = 0; index < statements->body.size; index++)
    last_evaluated_t_node_index =
      ti_eval_expression(context, statements->body.nodes[index], depth + 1);

  return last_evaluated_t_node_index;
}

uint16_t
ti_eval_expression(TiContext *context, const pm_node_t *node, int depth) {
  if (!node || depth > TI_EVAL_DEPTH_LIMIT || context->failed)
    return 0;

  switch (PM_NODE_TYPE(node)) {
  case PM_INTEGER_NODE:
    return new_builtin_t(TI_CLASS_INTEGER);

  case PM_FLOAT_NODE:
    return new_builtin_t(TI_CLASS_FLOAT);

  case PM_STRING_NODE:
  case PM_INTERPOLATED_STRING_NODE:
    return new_builtin_t(TI_CLASS_STRING);

  case PM_SYMBOL_NODE:
  case PM_INTERPOLATED_SYMBOL_NODE:
    return new_builtin_t(TI_CLASS_SYMBOL);

  case PM_TRUE_NODE:
    return new_builtin_t(TI_CLASS_TRUE);

  case PM_FALSE_NODE:
    return new_builtin_t(TI_CLASS_FALSE);

  case PM_NIL_NODE:
    return new_builtin_t(TI_CLASS_NIL);

  case PM_ARRAY_NODE:
    return ti_make_array(context, (const pm_array_node_t *)node, depth);

  case PM_LOCAL_VARIABLE_WRITE_NODE: {
    const pm_local_variable_write_node_t *write =
      (const pm_local_variable_write_node_t *)node;
    return ti_bind_scalar_assignment(context, write->name, write->value, depth);
  }

  case PM_INSTANCE_VARIABLE_WRITE_NODE: {
    const pm_instance_variable_write_node_t *write =
      (const pm_instance_variable_write_node_t *)node;
    return ti_bind_scalar_assignment(context, write->name, write->value, depth);
  }

  case PM_GLOBAL_VARIABLE_WRITE_NODE: {
    const pm_global_variable_write_node_t *write =
      (const pm_global_variable_write_node_t *)node;
    return ti_bind_scalar_assignment(context, write->name, write->value, depth);
  }

  case PM_CONSTANT_WRITE_NODE: {
    const pm_constant_write_node_t *write =
      (const pm_constant_write_node_t *)node;
    return ti_bind_scalar_assignment(context, write->name, write->value, depth);
  }

  case PM_HASH_NODE:
  case PM_KEYWORD_HASH_NODE:
    return new_builtin_t(TI_CLASS_HASH);

  case PM_RANGE_NODE:
    return new_builtin_t(TI_CLASS_RANGE);

  case PM_LAMBDA_NODE:
  case PM_BLOCK_NODE:
    return new_builtin_t(TI_CLASS_PROC);

  case PM_LOCAL_VARIABLE_READ_NODE:
    return ti_handle_identifier(
      ((const pm_local_variable_read_node_t *)node)->name
    );

  case PM_INSTANCE_VARIABLE_READ_NODE:
    return ti_handle_identifier(
      ((const pm_instance_variable_read_node_t *)node)->name
    );

  case PM_GLOBAL_VARIABLE_READ_NODE:
    return ti_handle_identifier(
      ((const pm_global_variable_read_node_t *)node)->name
    );

  case PM_CONSTANT_READ_NODE:
    return ti_handle_const_evaluation(
      context,
      (const pm_constant_read_node_t *)node
    );

  case PM_CALL_NODE:
    return ti_eval_method(context, (const pm_call_node_t *)node, depth);

  case PM_PARENTHESES_NODE:
    return ti_eval_statements(
      context,
      (const pm_statements_node_t *)
        ((const pm_parentheses_node_t *)node)->body,
      depth + 1
    );

  case PM_IF_NODE:
    return ti_eval_ifunless(context, (const pm_if_node_t *)node, depth);

  case PM_RETURN_NODE:
    return ti_eval_return(context, (const pm_return_node_t *)node, depth);

  default:
    return 0;
  }
}

static bool
eval_on_visit(const pm_node_t *node, void *data) {
  TiContext *context = data;

  if (context->failed)
    return false;

  switch (PM_NODE_TYPE(node)) {
  case PM_LOCAL_VARIABLE_WRITE_NODE: {
    const pm_local_variable_write_node_t *write =
      (const pm_local_variable_write_node_t *)node;

    ti_bind_scalar_assignment(context, write->name, write->value, 0);

    break;
  }

  case PM_INSTANCE_VARIABLE_WRITE_NODE: {
    const pm_instance_variable_write_node_t *write =
      (const pm_instance_variable_write_node_t *)node;

    ti_bind_scalar_assignment(context, write->name, write->value, 0);

    break;
  }

  case PM_GLOBAL_VARIABLE_WRITE_NODE: {
    const pm_global_variable_write_node_t *write =
      (const pm_global_variable_write_node_t *)node;

    ti_bind_scalar_assignment(context, write->name, write->value, 0);
    break;
  }

  case PM_CONSTANT_WRITE_NODE: {
    const pm_constant_write_node_t *write =
      (const pm_constant_write_node_t *)node;

    ti_bind_scalar_assignment(context, write->name, write->value, 0);

    break;
  }

  case PM_DEF_NODE:
    ti_eval_def(context, (const pm_def_node_t *)node);
    break;

  case PM_CLASS_NODE: {
    const pm_class_node_t *class_node = (const pm_class_node_t *)node;
    uint16_t class_name_id;

    if (!ti_convert_constant_id(class_node->name, &class_name_id)) {
      context->failed = 1;
      return false;
    }

    ti_eval_class(context, class_node);

    uint16_t outer_class_name_id = context->current_class_name_id;
    context->current_class_name_id = class_name_id;

    if (!context->failed && class_node->body)
      pm_visit_node(class_node->body, eval_on_visit, context);

    context->current_class_name_id = outer_class_name_id;
    return false;
  }

  default:
    break;
  }

  return !context->failed;
}

int
ti_evaluation_loop(TiContext *context, const pm_node_t *root) {
  ti_reset_arena();

  if (!ti_initialize_t() || !ti_initialize_t_frame() ||
      !ti_initialize_define_infos()) {
    return 0;
  }

  context->round = 1;
  pm_visit_node(root, eval_on_visit, context);

  if (!context->failed) {
    context->round = 2;
    pm_visit_node(root, eval_on_visit, context);
  }

  return !context->failed;
}
