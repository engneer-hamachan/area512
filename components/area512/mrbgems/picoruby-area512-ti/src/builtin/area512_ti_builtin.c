#include "area512_ti_builtin.h"
#include <string.h>

static int
compare_pool_string(
  const char *pool_string,
  const uint8_t *bytes,
  size_t byte_length
) {
  size_t pool_length = strlen(pool_string);
  size_t common_length = pool_length < byte_length ? pool_length : byte_length;
  int comparison = memcmp(pool_string, bytes, common_length);

  if (comparison != 0)
    return comparison;
  if (pool_length < byte_length)
    return -1;
  if (pool_length > byte_length)
    return 1;

  return 0;
}

uint8_t
ti_get_builtin_class_id(const uint8_t *name, size_t length) {
  if (!name || length == 0)
    return TI_CLASS_NONE;

  for (uint8_t class_id = 1; class_id < ti_builtin_class_count; class_id++) {
    const TiBuiltinClass *class_entry = &ti_builtin_classes[class_id];
    const char *class_name = ti_builtin_name_pool + class_entry->name_offset;

    if (compare_pool_string(class_name, name, length) == 0)
      return class_id;
  }

  return TI_CLASS_NONE;
}

static void
get_builtin_method_range(
  uint8_t class_id,
  int use_static_methods,
  uint16_t *start,
  uint16_t *count
) {
  *start = 0;
  *count = 0;

  if (class_id == TI_CLASS_NONE || class_id >= ti_builtin_class_count)
    return;

  const TiBuiltinClass *class_entry = &ti_builtin_classes[class_id];

  if (use_static_methods) {
    *start = class_entry->static_method_start;
    *count = class_entry->static_method_count;
  } else {
    *start = class_entry->instance_method_start;
    *count = class_entry->instance_method_count;
  }
}

static const TiBuiltinMethod *
find_builtin_method_in_range(
  uint16_t start,
  uint16_t count,
  const uint8_t *name,
  size_t length
) {
  uint16_t lower = 0;
  uint16_t upper = count;

  while (lower < upper) {
    uint16_t middle = lower + (uint16_t)((upper - lower) / 2U);
    const TiBuiltinMethod *method = &ti_builtin_methods[start + middle];
    int comparison =
      compare_pool_string(ti_get_builtin_method_name(method), name, length);

    if (comparison < 0)
      lower = middle + 1U;
    else
      upper = middle;
  }

  if (lower >= count)
    return NULL;

  const TiBuiltinMethod *method = &ti_builtin_methods[start + lower];
  if (compare_pool_string(ti_get_builtin_method_name(method), name, length) !=
      0) {
    return NULL;
  }

  return method;
}

const TiBuiltinMethod *
ti_get_builtin_instance_method(
  uint8_t class_id,
  const uint8_t *name,
  size_t length
) {
  uint16_t start;
  uint16_t count;
  get_builtin_method_range(class_id, 0, &start, &count);

  return find_builtin_method_in_range(start, count, name, length);
}

const TiBuiltinMethod *
ti_get_builtin_static_method(
  uint8_t class_id,
  const uint8_t *name,
  size_t length
) {
  uint16_t start;
  uint16_t count;
  get_builtin_method_range(class_id, 1, &start, &count);

  return find_builtin_method_in_range(start, count, name, length);
}

static int
has_prefix(const char *name, const uint8_t *prefix, size_t prefix_length) {
  return strlen(name) >= prefix_length &&
         memcmp(name, prefix, prefix_length) == 0;
}

int
ti_collect_builtin_methods_with_prefix(
  uint8_t class_id,
  int use_static_methods,
  const uint8_t *prefix,
  size_t prefix_length,
  const TiBuiltinMethod **out,
  int out_capacity
) {
  if (!out || out_capacity <= 0)
    return 0;

  uint16_t start;
  uint16_t count;
  get_builtin_method_range(class_id, use_static_methods, &start, &count);

  uint16_t first = 0;
  if (prefix_length > 0) {
    uint16_t upper = count;

    while (first < upper) {
      uint16_t middle = first + (uint16_t)((upper - first) / 2U);
      const TiBuiltinMethod *method = &ti_builtin_methods[start + middle];
      int comparison = compare_pool_string(
        ti_get_builtin_method_name(method),
        prefix,
        prefix_length
      );

      if (comparison < 0)
        first = middle + 1U;
      else
        upper = middle;
    }
  }

  int result_count = 0;

  for (uint16_t index = first; index < count; index++) {
    const TiBuiltinMethod *method = &ti_builtin_methods[start + index];

    if (prefix_length > 0 &&
        !has_prefix(ti_get_builtin_method_name(method), prefix, prefix_length))
      break;

    out[result_count++] = method;
    if (result_count >= out_capacity)
      break;
  }

  return result_count;
}

const char *
ti_get_builtin_method_name(const TiBuiltinMethod *method) {
  return ti_builtin_name_pool + method->name_offset;
}

const char *
ti_get_builtin_signature(const TiBuiltinMethod *method) {
  return ti_builtin_signature_pool + method->signature_offset;
}

const char *
ti_get_builtin_document(const TiBuiltinMethod *method) {
  return ti_builtin_document_pool + method->document_offset;
}

const char *
ti_get_builtin_class_name(uint8_t class_id) {
  if (class_id == TI_CLASS_NONE || class_id >= ti_builtin_class_count)
    return NULL;

  return ti_builtin_name_pool + ti_builtin_classes[class_id].name_offset;
}

static int
get_builtin_union(uint8_t union_index, uint8_t out_class_ids[4]) {
  if (union_index == 0 || union_index >= ti_builtin_union_count)
    return 0;

  int count = 0;
  const TiBuiltinUnion *union_entry = &ti_builtin_unions[union_index];

  while (count < 4 && union_entry->member_class_ids[count] != 0) {
    out_class_ids[count] = union_entry->member_class_ids[count];
    count++;
  }

  return count;
}

int
ti_get_builtin_return_classes(
  const TiBuiltinMethod *method,
  uint8_t out_class_ids[4]
) {
  if (!method || !out_class_ids)
    return 0;

  int count = get_builtin_union(method->return_union_index, out_class_ids);
  if (count > 0)
    return count;

  if (method->return_class_id == TI_CLASS_NONE)
    return 0;

  out_class_ids[0] = method->return_class_id;
  return 1;
}

int
ti_get_builtin_return_variant_classes(
  const TiBuiltinMethod *method,
  uint8_t out_class_ids[4]
) {
  if (!method || !out_class_ids)
    return 0;

  int count = get_builtin_union(method->variant_union_index, out_class_ids);
  if (count > 0)
    return count;

  if (method->return_variant_class_id == TI_CLASS_NONE)
    return 0;

  out_class_ids[0] = method->return_variant_class_id;
  return 1;
}
