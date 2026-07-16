#ifndef AREA512_TI_IFUNLESS_H
#define AREA512_TI_IFUNLESS_H

#include "area512_ti_context.h"
#include <stdint.h>

uint16_t
ti_eval_ifunless(TiContext *context, const pm_if_node_t *if_node, int depth);

#endif
