#ifndef AREA512_TI_DEFINE_INFO_H
#define AREA512_TI_DEFINE_INFO_H

#include <stdint.h>

#define TI_DEFINE_INFO_CAPACITY 64
#define TI_DEFINE_ARG_CAPACITY 8

typedef struct {
  uint16_t name_id;
  uint16_t owner_class_name_id;
  uint16_t return_t_index;
  uint16_t define_row;
  uint8_t define_arg_count;
  uint8_t is_class;
  uint16_t define_arg_name_ids[TI_DEFINE_ARG_CAPACITY];
} TiDefineInfo;

int ti_initialize_define_infos(void);
TiDefineInfo *
ti_set_define_info(
  uint16_t name_id,
  uint16_t owner_class_name_id,
  uint16_t define_row,
  int is_class
);
TiDefineInfo *ti_get_define_info(int index);
int ti_get_define_info_count(void);
uint8_t ti_get_defined_class_id(uint16_t name_id);

#endif
