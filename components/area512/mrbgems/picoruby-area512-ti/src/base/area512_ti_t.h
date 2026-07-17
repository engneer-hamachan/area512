#ifndef AREA512_TI_T_H
#define AREA512_TI_T_H

#include <stdint.h>

#define TI_T_FLAG_DEFINED_CLASS 1U
#define TI_T_FLAG_STATIC 2U
#define TI_T_CAPACITY 500

typedef struct {
  uint8_t object_class_id;
  uint8_t t_flags;
  uint16_t variants;
  uint16_t union_next;
} T;

int ti_initialize_t(void);
uint16_t ti_new_t(uint8_t object_class_id, uint8_t t_flags, uint16_t variants);
uint16_t
ti_make_union(uint16_t first_t_node_index, uint16_t second_t_node_index);
const T *ti_get_t(uint16_t t_node_index);
int ti_get_t_count(void);

#endif
