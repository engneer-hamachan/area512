#include "area512_ti_method_evaluator.h"
#include "area512_ti_builtin.h"
#include "area512_ti_eval.h"
#include "area512_ti_t.h"
#include "area512_ti_t_frame.h"
#include <string.h>

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
    uint16_t union_next_t_index =
      ti_new_t(return_variant_class_ids[index], 0, 0);
    return_variant_t_index =
      ti_make_union(return_variant_t_index, union_next_t_index);

    if (return_variant_t_index == 0)
      return 0;
  }

  uint16_t result = 0;

  for (int index = 0; index < return_class_count; index++) {
    uint16_t parameter =
      return_class_ids[index] == TI_CLASS_ARRAY ? return_variant_t_index : 0;
    uint16_t union_next_t_index =
      ti_new_t(return_class_ids[index], 0, parameter);
    result = ti_make_union(result, union_next_t_index);

    if (result == 0)
      return 0;
  }

  return result;
}

uint16_t
ti_eval_method(TiContext *context, const pm_call_node_t *call, int depth) {
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
      (receiver_t->t_flags & TI_T_FLAG_STATIC) != 0) {
    return ti_new_t(
      receiver_t->object_class_id,
      receiver_t->t_flags & TI_T_FLAG_DEFINED_CLASS,
      receiver_t->variants
    );
  }

  if ((receiver_t->t_flags & TI_T_FLAG_DEFINED_CLASS) != 0)
    return ti_get_value_t(name_id);

  const TiBuiltinMethod *method;

  if ((receiver_t->t_flags & TI_T_FLAG_STATIC) != 0) {
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
