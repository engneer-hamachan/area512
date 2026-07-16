#ifndef AREA512_TI_RETURN_H
#define AREA512_TI_RETURN_H

#include "area512_ti_context.h"
#include <stdint.h>

uint16_t
ti_eval_return(
  TiContext *context,
  const pm_return_node_t *return_node,
  int depth
);

#endif
