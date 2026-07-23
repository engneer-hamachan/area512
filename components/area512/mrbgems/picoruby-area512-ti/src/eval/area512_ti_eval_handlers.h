#ifndef AREA512_TI_EVAL_HANDLERS_H
#define AREA512_TI_EVAL_HANDLERS_H

#include "area512_ti_context.h"
#include <stdint.h>

uint16_t ti_handle_identifier(pm_constant_id_t constant_id);
uint16_t ti_handle_const_evaluation(
  TiContext *context,
  const pm_constant_read_node_t *constant_read
);

#endif
