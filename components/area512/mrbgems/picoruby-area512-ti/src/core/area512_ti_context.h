#ifndef AREA512_TI_CONTEXT_H
#define AREA512_TI_CONTEXT_H

#include <prism.h>

typedef struct {
  pm_parser_t *parser;
  const uint8_t *source;
  size_t source_length;
  int round;
  int failed;
} TiContext;

int ti_convert_constant_id(pm_constant_id_t constant_id, uint16_t *name_id);
const pm_constant_t *
ti_get_constant(const TiContext *context, pm_constant_id_t constant_id);

#endif
