#ifndef AREA512_TI_EVAL_EXPRESSION_H
#define AREA512_TI_EVAL_EXPRESSION_H

#include "area512_ti_context.h"
#include <stdint.h>

uint16_t
ti_eval_expression(TiContext *context, const pm_node_t *node, int depth);

#endif
