#include "area512_ti_ifunless.h"
#include "area512_ti_builtin.h"
#include "area512_ti_eval.h"
#include "area512_ti_t.h"

uint16_t
ti_eval_ifunless(TiContext *context, const pm_if_node_t *if_node, int depth) {
  ti_eval_expression(context, if_node->predicate, depth + 1);

  uint16_t result_t_node_index =
    ti_eval_statements(context, if_node->statements, depth + 1);

  if (result_t_node_index == 0)
    result_t_node_index = ti_new_t(TI_CLASS_NIL, 0, 0);

  uint16_t subsequent_t_node_index;

  if (if_node->subsequent) {
    //else
    if (PM_NODE_TYPE(if_node->subsequent) == PM_ELSE_NODE) {
      const pm_else_node_t *else_node =
        (const pm_else_node_t *)if_node->subsequent;

      subsequent_t_node_index =
        ti_eval_statements(context, else_node->statements, depth + 1);

    //elsif
    } else {
      subsequent_t_node_index =
        ti_eval_expression(context, if_node->subsequent, depth + 1);
    }
  // if true; end;
  } else {
    subsequent_t_node_index = ti_new_t(TI_CLASS_NIL, 0, 0);
  }

  return ti_make_union(result_t_node_index, subsequent_t_node_index);
}
