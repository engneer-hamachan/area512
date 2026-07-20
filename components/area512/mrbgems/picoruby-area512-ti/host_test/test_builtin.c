#include "area512_ti_builtin.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static void
test_string_method_lookup(void) {
  const TiBuiltinMethod *method =
    ti_get_builtin_instance_method(TI_CLASS_STRING, (const uint8_t *)"sub", 3);

  assert(method);
  assert(
    strcmp(
      ti_get_builtin_signature(method),
      "sub: (String arg0, String arg1) -> String"
    ) == 0
  );
}

static void
test_flattened_methods(void) {
  const TiBuiltinMethod *enumerable_method = ti_get_builtin_instance_method(
    TI_CLASS_ARRAY,
    (const uint8_t *)"each_with_index",
    15
  );
  assert(enumerable_method);
  assert(enumerable_method->origin_class_id == TI_CLASS_ENUMERABLE);

  const TiBuiltinMethod *object_method = ti_get_builtin_instance_method(
    TI_CLASS_STRING,
    (const uint8_t *)"class",
    5
  );
  assert(object_method);
  assert(object_method->origin_class_id == TI_CLASS_OBJECT);
}

static void
test_array_reference_return_type(void) {
  const TiBuiltinMethod *method =
    ti_get_builtin_instance_method(TI_CLASS_ARRAY, (const uint8_t *)"[]", 2);
  uint8_t return_class_ids[4] = {0};

  assert(method);
  assert(ti_get_builtin_return_classes(method, return_class_ids) == 1);
  assert(return_class_ids[0] == TI_CLASS_UNTYPED);
}

static void
test_union_return_type(void) {
  const TiBuiltinMethod *method =
    ti_get_builtin_instance_method(TI_CLASS_INTEGER, (const uint8_t *)"%", 1);
  uint8_t return_class_ids[4] = {0};

  assert(method);
  assert(ti_get_builtin_return_classes(method, return_class_ids) == 2);
  assert(return_class_ids[0] == TI_CLASS_INTEGER);
  assert(return_class_ids[1] == TI_CLASS_FLOAT);
}

static void
test_prefix_lookup(void) {
  const TiBuiltinMethod *methods[64];
  int count = ti_collect_builtin_methods_matching_partial_method_name(
    TI_CLASS_STRING,
    0,
    (const uint8_t *)"s",
    1,
    methods,
    64
  );

  assert(count > 0);

  int found_size = 0;
  int found_slice = 0;
  int found_split = 0;
  int found_sub = 0;
  for (int index = 0; index < count; index++) {
    const char *name = ti_get_builtin_method_name(methods[index]);

    assert(name[0] == 's');
    if (index > 0) {
      assert(strcmp(ti_get_builtin_method_name(methods[index - 1]), name) <= 0);
    }
    if (strcmp(name, "split") == 0)
      found_split = 1;
    if (strcmp(name, "size") == 0)
      found_size = 1;
    if (strcmp(name, "slice") == 0)
      found_slice = 1;
    if (strcmp(name, "sub") == 0)
      found_sub = 1;
  }

  assert(found_size);
  assert(found_slice);
  assert(found_split);
  assert(found_sub);
}

static void
test_pool_boundaries(void) {
  for (uint16_t class_id = 1; class_id < ti_builtin_class_count; class_id++) {
    const TiBuiltinClass *class_entry = &ti_builtin_classes[class_id];
    assert(class_entry->name_offset < ti_builtin_name_pool_size);
    assert(memchr(
      ti_builtin_name_pool + class_entry->name_offset,
      '\0',
      ti_builtin_name_pool_size - class_entry->name_offset
    ));
  }

  for (uint16_t index = 0; index < ti_builtin_method_count; index++) {
    const TiBuiltinMethod *method = &ti_builtin_methods[index];

    assert(method->name_offset < ti_builtin_name_pool_size);
    assert(method->signature_offset < ti_builtin_signature_pool_size);
    assert(method->document_offset < ti_builtin_document_pool_size);
    assert(memchr(
      ti_builtin_name_pool + method->name_offset,
      '\0',
      ti_builtin_name_pool_size - method->name_offset
    ));
    assert(memchr(
      ti_builtin_signature_pool + method->signature_offset,
      '\0',
      ti_builtin_signature_pool_size - method->signature_offset
    ));
    assert(memchr(
      ti_builtin_document_pool + method->document_offset,
      '\0',
      ti_builtin_document_pool_size - method->document_offset
    ));
  }
}

int
main(void) {
  test_string_method_lookup();
  test_flattened_methods();
  test_array_reference_return_type();
  test_union_return_type();
  test_prefix_lookup();
  test_pool_boundaries();

  return 0;
}
