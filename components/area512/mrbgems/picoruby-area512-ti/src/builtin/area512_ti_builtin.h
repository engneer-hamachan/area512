#ifndef AREA512_TI_BUILTIN_H
#define AREA512_TI_BUILTIN_H

#include "area512_ti_builtin_db.h"
#include <stddef.h>
#include <stdint.h>

uint8_t ti_get_builtin_class_id(const uint8_t *name, size_t length);
const TiBuiltinMethod *ti_get_builtin_instance_method(
  uint8_t class_id,
  const uint8_t *name,
  size_t length
);
const TiBuiltinMethod *ti_get_builtin_static_method(
  uint8_t class_id,
  const uint8_t *name,
  size_t length
);
int ti_collect_builtin_methods_with_prefix(
  uint8_t class_id,
  int use_static_methods,
  const uint8_t *prefix,
  size_t prefix_length,
  const TiBuiltinMethod **out,
  int out_capacity
);
const char *ti_get_builtin_method_name(const TiBuiltinMethod *method);
const char *ti_get_builtin_signature(const TiBuiltinMethod *method);
const char *ti_get_builtin_document(const TiBuiltinMethod *method);
const char *ti_get_builtin_class_name(uint8_t class_id);
int ti_get_builtin_return_classes(
  const TiBuiltinMethod *method,
  uint8_t out_class_ids[4]
);
int ti_get_builtin_return_variant_classes(
  const TiBuiltinMethod *method,
  uint8_t out_class_ids[4]
);

#endif
