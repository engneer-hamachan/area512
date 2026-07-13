#ifndef AREA512_TI_METHOD_EVALUATOR_H
#define AREA512_TI_METHOD_EVALUATOR_H

#include "area512_ti_context.h"
#include <stdint.h>

uint16_t
ti_eval_method(TiContext *context, const pm_call_node_t *call, int depth);

#endif
