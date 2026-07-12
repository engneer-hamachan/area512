#include "area512_ti_eval_expression.h"
#include "area512_ti_builtin.h"
#include "area512_ti_define_info.h"
#include "area512_ti_t.h"
#include "area512_ti_t_frame.h"
#include <string.h>

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
    return 0;

  return ti_eval_expression(
    context,
    statements->body.nodes[statements->body.size - 1],
    depth + 1
  );
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

static uint16_t
make_method_return(const TiBuiltinMethod *method) {
  uint8_t return_class_ids[4];
  uint8_t return_variant_class_ids[4];
  int return_class_count =
    ti_get_builtin_return_classes(method, return_class_ids);
  int return_variant_class_count =
    ti_get_builtin_return_variant_classes(method, return_variant_class_ids);
  uint16_t return_variant_t_index = 0;

  for (int index = 0; index < return_variant_class_count; index++) {
    uint16_t member = new_builtin_t(return_variant_class_ids[index]);
    return_variant_t_index = ti_make_union(return_variant_t_index, member);

    if (return_variant_t_index == 0)
      return 0;
  }

  uint16_t result = 0;

  for (int index = 0; index < return_class_count; index++) {
    uint16_t parameter =
      return_class_ids[index] == TI_CLASS_ARRAY ? return_variant_t_index : 0;
    uint16_t member = ti_new_t(return_class_ids[index], 0, parameter, 0);
    result = ti_make_union(result, member);

    if (result == 0)
      return 0;
  }

  return result;
}

static uint16_t
eval_method(TiContext *context, const pm_call_node_t *call, int depth) {
  uint16_t name_id;

  if (!ti_convert_constant_id(call->name, &name_id))
    return 0;

  if (!call->receiver)
    return ti_get_value_t(name_id);

  uint16_t receiver_t_index =
    ti_eval_expression(context, call->receiver, depth + 1);
  const T *receiver_t = ti_get_t(receiver_t_index);

  if (!receiver_t || receiver_t->union_next != 0)
    return 0;

  const pm_constant_t *name = ti_get_constant(context, call->name);
  if (!name)
    return 0;

  if (name->length == 3 && memcmp(name->start, "new", 3) == 0 &&
      (receiver_t->flags & TI_T_FLAG_STATIC) != 0) {
    return ti_new_t(
      receiver_t->object_class_id,
      receiver_t->flags & TI_T_FLAG_DEFINED_CLASS,
      receiver_t->variant1,
      receiver_t->variant2
    );
  }

  if ((receiver_t->flags & TI_T_FLAG_DEFINED_CLASS) != 0)
    return ti_get_value_t(name_id);

  const TiBuiltinMethod *method;

  if ((receiver_t->flags & TI_T_FLAG_STATIC) != 0) {
    method = ti_get_builtin_static_method(
      receiver_t->object_class_id,
      name->start,
      name->length
    );
  } else {
    method = ti_get_builtin_instance_method(
      receiver_t->object_class_id,
      name->start,
      name->length
    );
  }

  if (!method)
    return 0;

  return make_method_return(method);
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
    return eval_method(context, (const pm_call_node_t *)node, depth);

  case PM_PARENTHESES_NODE:
    return ti_eval_expression(
      context,
      ((const pm_parentheses_node_t *)node)->body,
      depth + 1
    );

  case PM_STATEMENTS_NODE:
    return eval_statements(context, (const pm_statements_node_t *)node, depth);

  default:
    return 0;
  }
}
