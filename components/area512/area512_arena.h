#pragma once

#include <stdint.h>

#if defined(AREA512_CYD_2432S028)
#define AREA512_ARENA_BYTE_SIZE (80 * 1024)

extern uint8_t *g_area512_arena;

void area512_allocate_arena(void);
#else
#define AREA512_ARENA_BYTE_SIZE (100 * 1024)

extern uint8_t g_area512_arena[AREA512_ARENA_BYTE_SIZE];
#endif

void area512_init_arena(void);
void area512_detach_arena(void);
void area512_record_arena_used_byte_size(int arena_used_byte_size);
int area512_metrics_arena_used_pct(void);
int area512_metrics_arena_free_byte_size(void);
