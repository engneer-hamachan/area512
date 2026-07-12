#ifndef AREA512_TI_ARENA_H
#define AREA512_TI_ARENA_H

#include <stddef.h>

#define TI_ARENA_SIZE (8 * 1024)

void ti_reset_arena(void);
void *ti_allocate_from_arena(size_t size);
int ti_did_arena_overflow(void);

#endif
