#ifndef AREA512_TI_EVAL_H
#define AREA512_TI_EVAL_H

#include "area512_ti_context.h"
#include <stdint.h>

#define TI_EVAL_DEPTH_LIMIT 8

int ti_evaluation_loop(TiContext *context, const pm_node_t *root);
uint16_t ti_eval_statements(
  TiContext *context,
  const pm_statements_node_t *statements,
  int depth
);
uint16_t
ti_eval_expression(TiContext *context, const pm_node_t *node, int depth);

#endif
