#include "area512_ti_t_frame.h"
#include "area512_ti_arena.h"
#include "area512_ti_t.h"
#include <stddef.h>
#include <string.h>

#define TI_T_FRAME_CAPACITY 512

typedef struct {
  uint16_t name_id;
  uint16_t t_node_index;
} t_frame_entry;

static t_frame_entry *t_frame;

int
ti_initialize_t_frame(void) {
  t_frame = ti_allocate_from_arena(sizeof(t_frame_entry) * TI_T_FRAME_CAPACITY);

  if (!t_frame)
    return 0;

  memset(t_frame, 0, sizeof(t_frame_entry) * TI_T_FRAME_CAPACITY);

  return 1;
}

static t_frame_entry *
find_t_frame_entry(uint16_t name_id) {
  unsigned int index = name_id % TI_T_FRAME_CAPACITY;

  for (int checked_slot_count = 0; checked_slot_count < TI_T_FRAME_CAPACITY;
       checked_slot_count++) {

    t_frame_entry *entry = &t_frame[index];

    if (entry->name_id == name_id || entry->name_id == 0)
      return entry;

    index = (index + 1U) % TI_T_FRAME_CAPACITY;
  }

  return NULL;
}

int
ti_set_value_t(uint16_t name_id, uint16_t t_node_index) {
  if (name_id == 0 || t_node_index == 0)
    return 1;

  t_frame_entry *entry = find_t_frame_entry(name_id);

  if (!entry)
    return 0;

  if (entry->name_id == 0) {
    entry->name_id = name_id;
    entry->t_node_index = t_node_index;

    return 1;
  }

  uint16_t union_t_node_index =
    ti_make_union(entry->t_node_index, t_node_index);

  if (union_t_node_index == 0)
    return 0;

  entry->t_node_index = union_t_node_index;

  return 1;
}

uint16_t
ti_get_value_t(uint16_t name_id) {
  if (!t_frame || name_id == 0)
    return 0;

  t_frame_entry *entry = find_t_frame_entry(name_id);

  if (!entry || entry->name_id != name_id)
    return 0;

  return entry->t_node_index;
}
