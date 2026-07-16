#ifndef AREA512_TI_SQUARE_BRACKET_H
#define AREA512_TI_SQUARE_BRACKET_H

#include "area512_ti_context.h"
#include <stdint.h>

uint16_t
ti_make_array(TiContext *context, const pm_array_node_t *array, int depth);

#endif
