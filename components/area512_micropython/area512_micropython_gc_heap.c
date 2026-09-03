#include "area512_micropython_gc_heap.h"
#include "py/mpstate.h"

#include <esp_heap_caps.h>

#include <stdlib.h>

#define MICROPYTHON_GC_HEAP_MARGIN_BYTE_SIZE (32 * 1024)
#define MICROPYTHON_GC_HEAP_MINIMUM_BYTE_SIZE (4 * 1024)
#define MICROPYTHON_GC_HEAP_MAXIMUM_BYTE_SIZE (192 * 1024)

void *
area512_micropython_allocate_gc_heap(size_t *gc_heap_byte_size) {
  size_t largest_free_byte_size =
    heap_caps_get_largest_free_block(MALLOC_CAP_DMA);

  size_t heap_byte_size =
    largest_free_byte_size > MICROPYTHON_GC_HEAP_MARGIN_BYTE_SIZE
      ? largest_free_byte_size - MICROPYTHON_GC_HEAP_MARGIN_BYTE_SIZE
      : MICROPYTHON_GC_HEAP_MINIMUM_BYTE_SIZE;

  if (heap_byte_size > MICROPYTHON_GC_HEAP_MAXIMUM_BYTE_SIZE)
    heap_byte_size = MICROPYTHON_GC_HEAP_MAXIMUM_BYTE_SIZE;

  while (heap_byte_size >= MICROPYTHON_GC_HEAP_MINIMUM_BYTE_SIZE) {
    void *gc_heap = malloc(heap_byte_size);

    if (gc_heap) {
      *gc_heap_byte_size = heap_byte_size;

      return gc_heap;
    }

    heap_byte_size /= 2;
  }

  return NULL;
}

void
area512_micropython_free_gc_heap_split_areas(void) {
  mp_state_mem_area_t *area = MP_STATE_MEM(area).next;

  MP_STATE_MEM(area).next = NULL;

  while (area) {
    mp_state_mem_area_t *next_area = area->next;

    free(area);

    area = next_area;
  }
}
