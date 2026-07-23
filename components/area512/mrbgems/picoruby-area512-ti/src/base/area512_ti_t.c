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
is_equal_t_with_next(
  const T *node,
  uint8_t object_class_id,
  uint8_t t_flags,
  uint16_t variants,
  uint16_t union_next
) {

  return node->object_class_id == object_class_id && node->t_flags == t_flags &&
         node->variants == variants && node->union_next == union_next;
}

static uint16_t
new_t_with_next(
  uint8_t object_class_id,
  uint8_t t_flags,
  uint16_t variants,
  uint16_t union_next
) {

  for (uint16_t index = 1; index < t_count; index++) {
    if (is_equal_t_with_next(
          &t_nodes[index],
          object_class_id,
          t_flags,
          variants,
          union_next
        )) {

      return index;
    }
  }

  if (t_count >= TI_T_CAPACITY)
    return 0;

  uint16_t index = t_count++;

  t_nodes[index].object_class_id = object_class_id;
  t_nodes[index].t_flags = t_flags;
  t_nodes[index].variants = variants;
  t_nodes[index].union_next = union_next;

  return index;
}

uint16_t
ti_new_t(uint8_t object_class_id, uint8_t t_flags, uint16_t variants) {

  if (object_class_id == 0)
    return 0;

  return new_t_with_next(object_class_id, t_flags, variants, 0);
}

static int
is_equal_t(const T *first, const T *second) {
  return first->object_class_id == second->object_class_id &&
         first->t_flags == second->t_flags &&
         first->variants == second->variants;
}

static int
contains_union_next(uint16_t t_node_index, const T *union_next_t) {
  while (t_node_index != 0) {
    const T *current = &t_nodes[t_node_index];

    if (is_equal_t(current, union_next_t))
      return 1;

    t_node_index = current->union_next;
  }

  return 0;
}

static uint16_t
append_union_next(uint16_t t_node_index, const T *union_next_t) {
  uint16_t indexes[TI_T_CAPACITY];
  uint16_t count = 0;

  while (t_node_index != 0) {
    if (count >= TI_T_CAPACITY)
      return 0;

    indexes[count++] = t_node_index;
    t_node_index = t_nodes[t_node_index].union_next;
  }

  uint16_t union_next_t_node_index = new_t_with_next(
    union_next_t->object_class_id,
    union_next_t->t_flags,
    union_next_t->variants,
    0
  );

  if (union_next_t_node_index == 0)
    return 0;

  while (count > 0) {
    const T *current = &t_nodes[indexes[--count]];

    union_next_t_node_index = new_t_with_next(
      current->object_class_id,
      current->t_flags,
      current->variants,
      union_next_t_node_index
    );

    if (union_next_t_node_index == 0)
      return 0;
  }

  return union_next_t_node_index;
}

uint16_t
ti_make_union(uint16_t first_t_node_index, uint16_t second_t_node_index) {
  if (first_t_node_index == 0)
    return second_t_node_index;

  if (second_t_node_index == 0 || first_t_node_index == second_t_node_index)
    return first_t_node_index;

  uint16_t result = first_t_node_index;
  uint16_t union_next_t_node_index = second_t_node_index;

  while (union_next_t_node_index != 0) {
    T union_next_t = t_nodes[union_next_t_node_index];

    if (!contains_union_next(result, &union_next_t)) {
      result = append_union_next(result, &union_next_t);

      if (result == 0)
        return 0;
    }

    union_next_t_node_index = union_next_t.union_next;
  }

  return result;
}

const T *
ti_get_t(uint16_t t_node_index) {
  if (t_node_index == 0 || t_node_index >= t_count)
    return NULL;

  return &t_nodes[t_node_index];
}

int
ti_get_t_count(void) {
  return t_count;
}
