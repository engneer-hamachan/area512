#include "area512_ti_method_evaluator.h"
#include "area512_ti_builtin.h"
#include "area512_ti_eval.h"
#include "area512_ti_t.h"
#include "area512_ti_t_frame.h"
#include <string.h>

static int
make_array_variants(const TiBuiltinMethod *method, uint16_t *variants) {
  uint8_t class_ids[4];

  int class_count =
    ti_get_builtin_return_array_variant_classes(method, class_ids);

  *variants = 0;

  for (int index = 0; index < class_count; index++) {
    uint16_t t_node_index = ti_new_t(class_ids[index], 0, 0);

    *variants = ti_make_union(*variants, t_node_index);

    if (*variants == 0)
      return 0;
  }

  return 1;
}

static uint16_t
make_method_return(const TiBuiltinMethod *method) {
  uint8_t return_class_ids[4];

  int return_class_count =
    ti_get_builtin_return_classes(method, return_class_ids);

  uint16_t result = 0;

  for (int index = 0; index < return_class_count; index++) {
    uint16_t variants = 0;

    if (
        return_class_ids[index] == TI_CLASS_ARRAY &&
        !make_array_variants(method, &variants)
      ) {

      return 0;
    }

    uint16_t union_next_t_node_index =
      ti_new_t(return_class_ids[index], 0, variants);

    result = ti_make_union(result, union_next_t_node_index);

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

  uint16_t receiver_t_node_index =
    ti_eval_expression(context, call->receiver, depth + 1);

  const T *receiver_t = ti_get_t(receiver_t_node_index);

  if (!receiver_t || receiver_t->union_next != 0)
    return 0;

  const pm_constant_t *method_name = ti_get_constant(context, call->name);

  if (!method_name)
    return 0;

  if (method_name->length == 3 &&
      memcmp(method_name->start, "new", 3) == 0 &&
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
    method =
      ti_get_builtin_static_method(
        receiver_t->object_class_id,
        method_name->start,
        method_name->length
      );
  } else {
    method =
      ti_get_builtin_instance_method(
        receiver_t->object_class_id,
        method_name->start,
        method_name->length
      );
  }

  if (!method)
    return 0;

  return make_method_return(method);
}
