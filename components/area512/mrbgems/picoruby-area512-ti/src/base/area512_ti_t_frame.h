#ifndef AREA512_TI_T_FRAME_H
#define AREA512_TI_T_FRAME_H

#include <stdint.h>

int ti_initialize_t_frame(void);
int ti_set_value_t(uint16_t name_id, uint16_t t_node_index);
uint16_t ti_get_value_t(uint16_t name_id);

#endif
