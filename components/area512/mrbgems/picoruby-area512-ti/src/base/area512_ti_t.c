#include "area512_ti_t.h"
#include "area512_ti_arena.h"
#include <stddef.h>

static T *t_nodes;
static uint16_t t_count;

int
ti_initialize_t(void) {
  t_nodes = ti_allocate_from_arena(sizeof(T) * TI_T_CAPACITY);

  if (!t_nodes)
    return 0;

  t_count = 1;
  t_nodes[0] = (T){0};

  return 1;
}

static int
matches_type(
  const T *node,
  uint8_t object_class_id,
  uint8_t flags,
  uint16_t variant1,
  uint16_t variant2,
  uint16_t union_next
) {
  return node->object_class_id == object_class_id && node->flags == flags &&
         node->variant1 == variant1 && node->variant2 == variant2 &&
         node->union_next == union_next;
}

static uint16_t
new_t_with_next(
  uint8_t object_class_id,
  uint8_t flags,
  uint16_t variant1,
  uint16_t variant2,
  uint16_t union_next
) {
  for (uint16_t index = 1; index < t_count; index++) {
    if (matches_type(
          &t_nodes[index],
          object_class_id,
          flags,
          variant1,
          variant2,
          union_next
        )) {
      return index;
    }
  }

  if (t_count >= TI_T_CAPACITY)
    return 0;

  uint16_t index = t_count++;
  t_nodes[index].object_class_id = object_class_id;
  t_nodes[index].flags = flags;
  t_nodes[index].variant1 = variant1;
  t_nodes[index].variant2 = variant2;
  t_nodes[index].union_next = union_next;

  return index;
}

uint16_t
ti_new_t(
  uint8_t object_class_id,
  uint8_t flags,
  uint16_t variant1,
  uint16_t variant2
) {
  if (object_class_id == 0)
    return 0;

  return new_t_with_next(object_class_id, flags, variant1, variant2, 0);
}

static int
compare_types(const T *first, const T *second) {
  if (first->object_class_id != second->object_class_id)
    return (int)first->object_class_id - (int)second->object_class_id;
  if (first->flags != second->flags)
    return (int)first->flags - (int)second->flags;
  if (first->variant1 != second->variant1)
    return (int)first->variant1 - (int)second->variant1;
  return (int)first->variant2 - (int)second->variant2;
}

static int
contains_variant(uint16_t t_index, const T *member) {
  while (t_index != 0) {
    const T *current = &t_nodes[t_index];

    if (compare_types(current, member) == 0)
      return 1;

    t_index = current->union_next;
  }

  return 0;
}

static uint16_t
append_variant(uint16_t t_index, const T *member) {
  if (t_index == 0) {
    return new_t_with_next(
      member->object_class_id,
      member->flags,
      member->variant1,
      member->variant2,
      0
    );
  }

  const T *current = &t_nodes[t_index];
  uint16_t next = append_variant(current->union_next, member);

  if (next == 0)
    return 0;

  return new_t_with_next(
    current->object_class_id,
    current->flags,
    current->variant1,
    current->variant2,
    next
  );
}

uint16_t
ti_make_union(uint16_t first_t_index, uint16_t second_t_index) {
  if (first_t_index == 0)
    return second_t_index;
  if (second_t_index == 0 || first_t_index == second_t_index)
    return first_t_index;

  uint16_t result = first_t_index;
  uint16_t member_index = second_t_index;

  while (member_index != 0) {
    T member = t_nodes[member_index];

    if (!contains_variant(result, &member)) {
      result = append_variant(result, &member);
      if (result == 0)
        return 0;
    }

    member_index = member.union_next;
  }

  return result;
}

const T *
ti_get_t(uint16_t t_index) {
  if (t_index == 0 || t_index >= t_count)
    return NULL;

  return &t_nodes[t_index];
}

int
ti_get_t_count(void) {
  return t_count;
}
