#include "area512_ti_eval_expression.h"
#include "area512_ti_builtin.h"
#include "area512_ti_bind.h"
#include "area512_ti_define_info.h"
#include "area512_ti_method_evaluator.h"
#include "area512_ti_t.h"
#include "area512_ti_t_frame.h"

#define TI_EVAL_DEPTH_LIMIT 8

static uint16_t
new_builtin_t(uint8_t class_id) {
  return ti_new_t(class_id, 0, 0, 0);
}

static uint16_t
eval_statements(
  TiContext *context,
  const pm_statements_node_t *statements,
  int depth
) {
  if (statements->body.size == 0)
    return new_builtin_t(TI_CLASS_NIL);

  uint16_t last_t_index = new_builtin_t(TI_CLASS_NIL);

  for (size_t index = 0; index < statements->body.size; index++)
    last_t_index = ti_eval_expression(
      context,
      statements->body.nodes[index],
      depth + 1
    );

  return last_t_index;
}

static uint16_t
eval_if(TiContext *context, const pm_if_node_t *if_node, int depth) {
  ti_eval_expression(context, if_node->predicate, depth + 1);

  uint16_t result_t_index =
    ti_eval_expression(context, (const pm_node_t *)if_node->statements, depth + 1);

  if (result_t_index == 0)
    result_t_index = new_builtin_t(TI_CLASS_NIL);

  uint16_t subsequent_t_index;
  if (if_node->subsequent) {
    subsequent_t_index =
      ti_eval_expression(context, if_node->subsequent, depth + 1);
  } else {
    subsequent_t_index = new_builtin_t(TI_CLASS_NIL);
  }

  return ti_make_union(result_t_index, subsequent_t_index);
}

static uint16_t
eval_return(TiContext *context, const pm_return_node_t *return_node, int depth) {
  uint16_t return_t_index = new_builtin_t(TI_CLASS_NIL);

  if (return_node->arguments && return_node->arguments->arguments.size > 0) {
    return_t_index = 0;

    for (size_t index = 0;
         index < return_node->arguments->arguments.size;
         index++) {
      uint16_t argument_t_index = ti_eval_expression(
        context,
        return_node->arguments->arguments.nodes[index],
        depth + 1
      );
      return_t_index = ti_make_union(return_t_index, argument_t_index);
    }
  }

  context->return_t_index =
    ti_make_union(context->return_t_index, return_t_index);

  return return_t_index;
}

static uint16_t
eval_array(TiContext *context, const pm_array_node_t *array, int depth) {
  uint16_t element_t_index = 0;
  int has_unknown_element = 0;

  for (size_t index = 0; index < array->elements.size; index++) {
    uint16_t current_t_index =
      ti_eval_expression(context, array->elements.nodes[index], depth + 1);

    if (current_t_index == 0) {
      has_unknown_element = 1;
      break;
    }

    if (element_t_index == 0) {
      element_t_index = current_t_index;
    } else if (element_t_index != current_t_index) {
      has_unknown_element = 1;
      break;
    }
  }

  if (has_unknown_element || element_t_index == 0) {
    element_t_index = new_builtin_t(TI_CLASS_UNTYPED);
  }

  return ti_new_t(TI_CLASS_ARRAY, 0, element_t_index, 0);
}

static uint16_t
eval_variable_read(pm_constant_id_t constant_id) {
  uint16_t name_id;

  if (!ti_convert_constant_id(constant_id, &name_id))
    return 0;

  return ti_get_value_t(name_id);
}

static uint16_t
eval_constant_read(
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
    return eval_array(context, (const pm_array_node_t *)node, depth);

  case PM_LOCAL_VARIABLE_WRITE_NODE: {
    const pm_local_variable_write_node_t *write =
      (const pm_local_variable_write_node_t *)node;
    return ti_bind_scalar_assignment(
      context,
      write->name,
      write->value,
      depth
    );
  }

  case PM_INSTANCE_VARIABLE_WRITE_NODE: {
    const pm_instance_variable_write_node_t *write =
      (const pm_instance_variable_write_node_t *)node;
    return ti_bind_scalar_assignment(
      context,
      write->name,
      write->value,
      depth
    );
  }

  case PM_GLOBAL_VARIABLE_WRITE_NODE: {
    const pm_global_variable_write_node_t *write =
      (const pm_global_variable_write_node_t *)node;
    return ti_bind_scalar_assignment(
      context,
      write->name,
      write->value,
      depth
    );
  }

  case PM_CONSTANT_WRITE_NODE: {
    const pm_constant_write_node_t *write =
      (const pm_constant_write_node_t *)node;
    return ti_bind_scalar_assignment(
      context,
      write->name,
      write->value,
      depth
    );
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
    return eval_variable_read(
      ((const pm_local_variable_read_node_t *)node)->name
    );

  case PM_INSTANCE_VARIABLE_READ_NODE:
    return eval_variable_read(
      ((const pm_instance_variable_read_node_t *)node)->name
    );

  case PM_GLOBAL_VARIABLE_READ_NODE:
    return eval_variable_read(
      ((const pm_global_variable_read_node_t *)node)->name
    );

  case PM_CONSTANT_READ_NODE:
    return eval_constant_read(context, (const pm_constant_read_node_t *)node);

  case PM_CALL_NODE:
    return ti_eval_method(context, (const pm_call_node_t *)node, depth);

  case PM_PARENTHESES_NODE:
    return ti_eval_expression(
      context,
      ((const pm_parentheses_node_t *)node)->body,
      depth + 1
    );

  case PM_STATEMENTS_NODE:
    return eval_statements(context, (const pm_statements_node_t *)node, depth);

  case PM_IF_NODE:
    return eval_if(context, (const pm_if_node_t *)node, depth);

  case PM_ELSE_NODE:
    return ti_eval_expression(
      context,
      (const pm_node_t *)((const pm_else_node_t *)node)->statements,
      depth + 1
    );

  case PM_RETURN_NODE:
    return eval_return(context, (const pm_return_node_t *)node, depth);

  default:
    return 0;
  }
}
