#ifndef AREA512_TI_BUILTIN_DB_H
#define AREA512_TI_BUILTIN_DB_H

#include <stdint.h>

typedef enum {
  TI_CLASS_NONE = 0,
  TI_CLASS_OBJECT,
  TI_CLASS_INTEGER,
  TI_CLASS_FLOAT,
  TI_CLASS_STRING,
  TI_CLASS_SYMBOL,
  TI_CLASS_ARRAY,
  TI_CLASS_HASH,
  TI_CLASS_RANGE,
  TI_CLASS_RNG,
  TI_CLASS_BOOL,
  TI_CLASS_TRUE,
  TI_CLASS_FALSE,
  TI_CLASS_NIL,
  TI_CLASS_PROC,
  TI_CLASS_KERNEL,
  TI_CLASS_ENUMERABLE,
  TI_CLASS_CLASS,
  TI_CLASS_DIR,
  TI_CLASS_FILE,
  TI_CLASS_MATH,
  TI_CLASS_GPIO,
  TI_CLASS_I2C,
  TI_CLASS_DISPLAY,
  TI_CLASS_SD,
  TI_CLASS_SANDBOX,
  TI_CLASS_SPRITE,
  TI_CLASS_WIDGET,
  TI_CLASS_WIDGET_LIST,
  TI_CLASS_WIDGET_TEXT_VIEW,
  TI_CLASS_RUNTIME_ERROR,
  TI_CLASS_UNTYPED,
  TI_CLASS_BUILTIN_COUNT,
  TI_CLASS_USER_BASE = TI_CLASS_BUILTIN_COUNT
} TiClassId;

typedef struct {
  uint16_t name_offset;
  uint16_t signature_offset;
  uint16_t document_offset;
  uint8_t return_class_id;
  uint8_t return_array_variant_class_id;
  uint8_t return_union_index;
  uint8_t array_variant_union_index;
  uint8_t block_parameter_class_id;
  uint8_t origin_class_id;
} TiBuiltinMethod;

typedef struct {
  uint16_t name_offset;
  uint16_t instance_method_start;
  uint16_t instance_method_count;
  uint16_t static_method_start;
  uint16_t static_method_count;
} TiBuiltinClass;

typedef struct {
  uint8_t member_class_ids[4];
} TiBuiltinUnion;

extern const TiBuiltinClass ti_builtin_classes[];
extern const TiBuiltinMethod ti_builtin_methods[];
extern const TiBuiltinUnion ti_builtin_unions[];
extern const char ti_builtin_name_pool[];
extern const char ti_builtin_signature_pool[];
extern const char ti_builtin_document_pool[];
extern const uint16_t ti_builtin_class_count;
extern const uint16_t ti_builtin_method_count;
extern const uint16_t ti_builtin_union_count;
extern const uint16_t ti_builtin_name_pool_size;
extern const uint16_t ti_builtin_signature_pool_size;
extern const uint16_t ti_builtin_document_pool_size;

#endif
