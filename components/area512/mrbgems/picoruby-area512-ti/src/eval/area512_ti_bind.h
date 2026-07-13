#ifndef AREA512_TI_BIND_H
#define AREA512_TI_BIND_H

#include "area512_ti_context.h"

void ti_bind_scalar_assignment(
  TiContext *context,
  pm_constant_id_t constant_id,
  const pm_node_t *value
);

#endif
