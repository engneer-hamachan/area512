#ifndef AREA512_TI_TYPE_H
#define AREA512_TI_TYPE_H

#include "area512_ti_context.h"
#include <stddef.h>
#include <stdint.h>

#define TI_TYPE_STRING_CAPACITY 96

int ti_type_to_string(
  const TiContext *context,
  uint16_t t_index,
  char *buffer,
  size_t capacity
);

#endif
