#include "area512_ti_square_bracket.h"
#include "area512_ti_builtin.h"
#include "area512_ti_eval.h"
#include "area512_ti_t.h"

uint16_t
ti_make_array(TiContext *context, const pm_array_node_t *array, int depth) {
  uint16_t variants = 0;
  int has_unknown_variant = 0;

  for (size_t index = 0; index < array->elements.size; index++) {
    uint16_t variant_t_node_index =
      ti_eval_expression(context, array->elements.nodes[index], depth + 1);

    if (variant_t_node_index == 0) {
      has_unknown_variant = 1;
      break;
    }

    variants = ti_make_union(variants, variant_t_node_index);

    if (variants == 0) {
      has_unknown_variant = 1;
      break;
    }
  }

  if (has_unknown_variant || variants == 0)
    variants = ti_new_t(TI_CLASS_UNTYPED, 0, 0);

  return ti_new_t(TI_CLASS_ARRAY, 0, variants);
}
