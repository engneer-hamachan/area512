#ifndef MICROPYTHON_TI_BUILTIN_DATABASE_H
#define MICROPYTHON_TI_BUILTIN_DATABASE_H

#include <stdint.h>

typedef enum {
  MICROPYTHON_TI_CLASS_NONE = 0,
  MICROPYTHON_TI_CLASS_OBJECT,
  MICROPYTHON_TI_CLASS_INT,
  MICROPYTHON_TI_CLASS_FLOAT,
  MICROPYTHON_TI_CLASS_STR,
  MICROPYTHON_TI_CLASS_BYTES,
  MICROPYTHON_TI_CLASS_BOOL,
  MICROPYTHON_TI_CLASS_TUPLE,
  MICROPYTHON_TI_CLASS_LIST,
  MICROPYTHON_TI_CLASS_DICT,
  MICROPYTHON_TI_CLASS_RANGE,
  MICROPYTHON_TI_CLASS_NONETYPE,
  MICROPYTHON_TI_CLASS_UNTYPED,
  MICROPYTHON_TI_CLASS_TYPE,
  MICROPYTHON_TI_CLASS_BUILTINS,
  MICROPYTHON_TI_CLASS_ADC,
  MICROPYTHON_TI_CLASS_CONSOLE,
  MICROPYTHON_TI_CLASS_DOT,
  MICROPYTHON_TI_CLASS_GPIO,
  MICROPYTHON_TI_CLASS_IO,
  MICROPYTHON_TI_CLASS_RNG,
  MICROPYTHON_TI_CLASS_SD,
  MICROPYTHON_TI_CLASS_FILE,
  MICROPYTHON_TI_CLASS_DIR,
  MICROPYTHON_TI_CLASS_SPRITE,
  MICROPYTHON_TI_CLASS_DISPLAY,
  MICROPYTHON_TI_CLASS_WIDGET,
  MICROPYTHON_TI_CLASS_WIDGETLIST,
  MICROPYTHON_TI_CLASS_WIDGETTEXTVIEW,
  MICROPYTHON_TI_CLASS_STATICMETHOD,
  MICROPYTHON_TI_CLASS_CLASSMETHOD,
  MICROPYTHON_TI_CLASS_SUPER,
  MICROPYTHON_TI_CLASS_MAP,
  MICROPYTHON_TI_CLASS_ZIP,
  MICROPYTHON_TI_CLASS_BASEEXCEPTION,
  MICROPYTHON_TI_CLASS_GENERATOREXIT,
  MICROPYTHON_TI_CLASS_KEYBOARDINTERRUPT,
  MICROPYTHON_TI_CLASS_SYSTEMEXIT,
  MICROPYTHON_TI_CLASS_EXCEPTION,
  MICROPYTHON_TI_CLASS_STOPITERATION,
  MICROPYTHON_TI_CLASS_OSERROR,
  MICROPYTHON_TI_CLASS_ARITHMETICERROR,
  MICROPYTHON_TI_CLASS_ASSERTIONERROR,
  MICROPYTHON_TI_CLASS_ATTRIBUTEERROR,
  MICROPYTHON_TI_CLASS_EOFERROR,
  MICROPYTHON_TI_CLASS_IMPORTERROR,
  MICROPYTHON_TI_CLASS_LOOKUPERROR,
  MICROPYTHON_TI_CLASS_MEMORYERROR,
  MICROPYTHON_TI_CLASS_NAMEERROR,
  MICROPYTHON_TI_CLASS_RUNTIMEERROR,
  MICROPYTHON_TI_CLASS_SYNTAXERROR,
  MICROPYTHON_TI_CLASS_TYPEERROR,
  MICROPYTHON_TI_CLASS_VALUEERROR,
  MICROPYTHON_TI_CLASS_OVERFLOWERROR,
  MICROPYTHON_TI_CLASS_ZERODIVISIONERROR,
  MICROPYTHON_TI_CLASS_INDEXERROR,
  MICROPYTHON_TI_CLASS_KEYERROR,
  MICROPYTHON_TI_CLASS_NOTIMPLEMENTEDERROR,
  MICROPYTHON_TI_CLASS_INDENTATIONERROR,
  MICROPYTHON_TI_CLASS_GC,
  MICROPYTHON_TI_CLASS_BUILTIN_COUNT,
  MICROPYTHON_TI_CLASS_USER_BASE = MICROPYTHON_TI_CLASS_BUILTIN_COUNT
} MicroPythonTiClassIdentifier;

typedef struct {
  uint16_t name_offset;
  uint16_t signature_offset;
  uint16_t document_offset;
  uint16_t argument_start_index;
  uint8_t argument_count;
  uint8_t return_class_identifier;
  uint8_t return_array_variant_class_identifier;
  uint16_t return_union_index;
  uint16_t array_variant_union_index;
  uint8_t block_parameter_class_identifier;
  uint8_t origin_class_identifier;
} MicroPythonTiBuiltinMethod;
typedef enum {
  MICROPYTHON_TI_BUILTIN_ARGUMENT_REQUIRED = 0,
  MICROPYTHON_TI_BUILTIN_ARGUMENT_OPTIONAL,
  MICROPYTHON_TI_BUILTIN_ARGUMENT_REST,
  MICROPYTHON_TI_BUILTIN_ARGUMENT_REQUIRED_KEYWORD,
  MICROPYTHON_TI_BUILTIN_ARGUMENT_OPTIONAL_KEYWORD,
  MICROPYTHON_TI_BUILTIN_ARGUMENT_REST_KEYWORD
} MicroPythonTiBuiltinArgumentKind;
typedef struct {
  uint16_t name_offset;
  uint8_t class_identifier;
  uint16_t union_index;
  uint8_t kind;
} MicroPythonTiBuiltinArgument;
typedef struct {
  uint16_t name_offset;
  uint16_t instance_method_start_index;
  uint16_t instance_method_count;
  uint16_t static_method_start_index;
  uint16_t static_method_count;
} MicroPythonTiBuiltinClass;
typedef struct {
  uint8_t member_class_identifiers[4];
} MicroPythonTiBuiltinUnion;

extern const MicroPythonTiBuiltinClass micropython_ti_builtin_classes[];
extern const MicroPythonTiBuiltinMethod micropython_ti_builtin_methods[];
extern const MicroPythonTiBuiltinArgument micropython_ti_builtin_arguments[];
extern const MicroPythonTiBuiltinUnion micropython_ti_builtin_unions[];
extern const char micropython_ti_builtin_name_pool[];
extern const char micropython_ti_builtin_signature_pool[];
extern const char micropython_ti_builtin_document_pool[];
extern const uint16_t micropython_ti_builtin_class_count;
extern const uint16_t micropython_ti_builtin_method_count;
extern const uint16_t micropython_ti_builtin_argument_count;
extern const uint16_t micropython_ti_builtin_union_count;
extern const uint16_t micropython_ti_builtin_name_pool_size;
extern const uint16_t micropython_ti_builtin_signature_pool_size;
extern const uint16_t micropython_ti_builtin_document_pool_size;

#endif
