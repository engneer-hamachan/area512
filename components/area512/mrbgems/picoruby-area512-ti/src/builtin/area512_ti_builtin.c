#include "area512_ti_builtin.h"
#include <string.h>

static int
compare_builtin_name(
  const char *builtin_name,
  const uint8_t *candidate_name,
  size_t candidate_name_length
) {
  size_t builtin_name_length = strlen(builtin_name);
  size_t comparison_length = builtin_name_length;

  if (candidate_name_length < comparison_length)
    comparison_length = candidate_name_length;

  int comparison =
    memcmp(builtin_name, candidate_name, comparison_length);

  if (comparison != 0)
    return comparison;
  if (builtin_name_length < candidate_name_length)
    return -1;
  if (builtin_name_length > candidate_name_length)
    return 1;

  return 0;
}

uint8_t
ti_get_builtin_class_id(
  const uint8_t *class_name,
  size_t class_name_length
) {
  if (!class_name || class_name_length == 0)
    return TI_CLASS_NONE;

  for (uint8_t class_id = 1; class_id < ti_builtin_class_count; class_id++) {
    const TiBuiltinClass *builtin_class = &ti_builtin_classes[class_id];
    const char *builtin_class_name =
      ti_builtin_name_pool + builtin_class->name_offset;

    if (
        compare_builtin_name(
          builtin_class_name,
          class_name,
          class_name_length
        ) == 0
    ) {
      return class_id;
    }
  }

  return TI_CLASS_NONE;
}

static void
get_builtin_method_range(
  uint8_t class_id,
  int use_static_methods,
  uint16_t *method_start_index,
  uint16_t *method_count
) {
  *method_start_index = 0;
  *method_count = 0;

  if (class_id == TI_CLASS_NONE || class_id >= ti_builtin_class_count)
    return;

  const TiBuiltinClass *builtin_class = &ti_builtin_classes[class_id];

  if (use_static_methods) {
    *method_start_index = builtin_class->static_method_start;
    *method_count = builtin_class->static_method_count;
  } else {
    *method_start_index = builtin_class->instance_method_start;
    *method_count = builtin_class->instance_method_count;
  }
}

static const TiBuiltinMethod *
find_builtin_method_in_range(
  uint16_t method_start_index,
  uint16_t method_count,
  const uint8_t *method_name,
  size_t method_name_length
) {
  uint16_t lower_bound_index = 0;
  uint16_t upper_bound_index = method_count;

  while (lower_bound_index < upper_bound_index) {
    uint16_t middle_index =
      lower_bound_index +
      (uint16_t)((upper_bound_index - lower_bound_index) / 2U);
    const TiBuiltinMethod *builtin_method =
      &ti_builtin_methods[method_start_index + middle_index];
    int comparison = compare_builtin_name(
      ti_get_builtin_method_name(builtin_method),
      method_name,
      method_name_length
    );

    if (comparison < 0)
      lower_bound_index = middle_index + 1U;
    else
      upper_bound_index = middle_index;
  }

  if (lower_bound_index >= method_count)
    return NULL;

  const TiBuiltinMethod *builtin_method =
    &ti_builtin_methods[method_start_index + lower_bound_index];
  if (
      compare_builtin_name(
        ti_get_builtin_method_name(builtin_method),
        method_name,
        method_name_length
      ) != 0
  ) {
    return NULL;
  }

  return builtin_method;
}

const TiBuiltinMethod *
ti_get_builtin_instance_method(
  uint8_t class_id,
  const uint8_t *method_name,
  size_t method_name_length
) {
  uint16_t method_start_index;
  uint16_t method_count;
  get_builtin_method_range(
    class_id,
    0,
    &method_start_index,
    &method_count
  );

  return find_builtin_method_in_range(
    method_start_index,
    method_count,
    method_name,
    method_name_length
  );
}

const TiBuiltinMethod *
ti_get_builtin_static_method(
  uint8_t class_id,
  const uint8_t *method_name,
  size_t method_name_length
) {
  uint16_t method_start_index;
  uint16_t method_count;
  get_builtin_method_range(
    class_id,
    1,
    &method_start_index,
    &method_count
  );

  return find_builtin_method_in_range(
    method_start_index,
    method_count,
    method_name,
    method_name_length
  );
}

static int
has_builtin_name_prefix(
  const char *builtin_name,
  const uint8_t *prefix,
  size_t prefix_length
) {
  return strlen(builtin_name) >= prefix_length &&
         memcmp(builtin_name, prefix, prefix_length) == 0;
}

int
ti_collect_builtin_methods_with_prefix(
  uint8_t class_id,
  int use_static_methods,
  const uint8_t *prefix,
  size_t prefix_length,
  const TiBuiltinMethod **output_methods,
  int output_capacity
) {
  if (!output_methods || output_capacity <= 0)
    return 0;

  uint16_t method_start_index;
  uint16_t method_count;
  get_builtin_method_range(
    class_id,
    use_static_methods,
    &method_start_index,
    &method_count
  );

  uint16_t first_matching_index = 0;
  if (prefix_length > 0) {
    uint16_t upper_bound_index = method_count;

    while (first_matching_index < upper_bound_index) {
      uint16_t middle_index =
        first_matching_index +
        (uint16_t)((upper_bound_index - first_matching_index) / 2U);
      const TiBuiltinMethod *builtin_method =
        &ti_builtin_methods[method_start_index + middle_index];
      int comparison = compare_builtin_name(
        ti_get_builtin_method_name(builtin_method),
        prefix,
        prefix_length
      );

      if (comparison < 0)
        first_matching_index = middle_index + 1U;
      else
        upper_bound_index = middle_index;
    }
  }

  int collected_method_count = 0;

  for (uint16_t index = first_matching_index; index < method_count; index++) {
    const TiBuiltinMethod *builtin_method =
      &ti_builtin_methods[method_start_index + index];

    if (prefix_length > 0 &&
        !has_builtin_name_prefix(
          ti_get_builtin_method_name(builtin_method),
          prefix,
          prefix_length
        )) {
      break;
    }

    output_methods[collected_method_count++] = builtin_method;
    if (collected_method_count >= output_capacity)
      break;
  }

  return collected_method_count;
}

const char *
ti_get_builtin_method_name(const TiBuiltinMethod *builtin_method) {
  return ti_builtin_name_pool + builtin_method->name_offset;
}

const char *
ti_get_builtin_signature(const TiBuiltinMethod *builtin_method) {
  return ti_builtin_signature_pool + builtin_method->signature_offset;
}

const char *
ti_get_builtin_document(const TiBuiltinMethod *builtin_method) {
  return ti_builtin_document_pool + builtin_method->document_offset;
}

const char *
ti_get_builtin_class_name(uint8_t class_id) {
  if (class_id == TI_CLASS_NONE || class_id >= ti_builtin_class_count)
    return NULL;

  return ti_builtin_name_pool + ti_builtin_classes[class_id].name_offset;
}

static int
copy_builtin_union_class_ids(
  uint8_t builtin_union_index,
  uint8_t output_class_ids[4]
) {
  if (
      builtin_union_index == 0 ||
      builtin_union_index >= ti_builtin_union_count
  ) {
    return 0;
  }

  int member_class_count = 0;
  const TiBuiltinUnion *builtin_union =
    &ti_builtin_unions[builtin_union_index];

  while (
      member_class_count < 4 &&
      builtin_union->member_class_ids[member_class_count] != 0
  ) {
    output_class_ids[member_class_count] =
      builtin_union->member_class_ids[member_class_count];
    member_class_count++;
  }

  return member_class_count;
}

int
ti_get_builtin_return_classes(
  const TiBuiltinMethod *builtin_method,
  uint8_t output_class_ids[4]
) {
  if (!builtin_method || !output_class_ids)
    return 0;

  int return_class_count = copy_builtin_union_class_ids(
    builtin_method->return_union_index,
    output_class_ids
  );
  if (return_class_count > 0)
    return return_class_count;

  if (builtin_method->return_class_id == TI_CLASS_NONE)
    return 0;

  output_class_ids[0] = builtin_method->return_class_id;
  return 1;
}

int
ti_get_builtin_return_variant_classes(
  const TiBuiltinMethod *builtin_method,
  uint8_t output_class_ids[4]
) {
  if (!builtin_method || !output_class_ids)
    return 0;

  int return_variant_class_count = copy_builtin_union_class_ids(
    builtin_method->variant_union_index,
    output_class_ids
  );
  if (return_variant_class_count > 0)
    return return_variant_class_count;

  if (builtin_method->return_variant_class_id == TI_CLASS_NONE)
    return 0;

  output_class_ids[0] = builtin_method->return_variant_class_id;
  return 1;
}
