#include "area512_ti_return.h"
#include "area512_ti_builtin.h"
#include "area512_ti_eval.h"
#include "area512_ti_t.h"

uint16_t
ti_eval_return(
  TiContext *context,
  const pm_return_node_t *return_node,
  int depth
) {
  uint16_t return_t_index = ti_new_t(TI_CLASS_NIL, 0, 0, 0);

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
