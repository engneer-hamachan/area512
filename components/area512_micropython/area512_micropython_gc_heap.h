#pragma once

#include <stddef.h>

void *area512_micropython_allocate_gc_heap(size_t *gc_heap_byte_size);

void area512_micropython_free_gc_heap_split_areas(void);
