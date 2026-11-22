#include "area512_arena.h"

#include <mrubyc.h>

#if defined(AREA512_CYD_2432S028)
#include <esp_heap_caps.h>
#include <stdlib.h>

uint8_t *g_area512_arena;
#else
uint8_t g_area512_arena[AREA512_ARENA_BYTE_SIZE] __attribute__((aligned(8)));
#endif

static int s_recorded_arena_used_byte_size;

#if defined(AREA512_CYD_2432S028)
void
area512_allocate_arena(void) {
  g_area512_arena =
    heap_caps_aligned_alloc(
      8,
      AREA512_ARENA_BYTE_SIZE,
      MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT
    );

  if (g_area512_arena == NULL)
    abort();
}
#endif

void
area512_init_arena(void) {
  mrbc_init(g_area512_arena, AREA512_ARENA_BYTE_SIZE);
}

void
area512_detach_arena(void) {
  struct MRBC_ALLOC_STATISTICS statistics;

  mrbc_alloc_statistics(&statistics);
  area512_record_arena_used_byte_size((int)statistics.used);

  mrbc_cleanup();
}

void
area512_record_arena_used_byte_size(int arena_used_byte_size) {
  if (arena_used_byte_size > AREA512_ARENA_BYTE_SIZE)
    arena_used_byte_size = AREA512_ARENA_BYTE_SIZE;

  s_recorded_arena_used_byte_size = arena_used_byte_size;
}

int
area512_metrics_arena_used_pct(void) {
  return s_recorded_arena_used_byte_size * 100 / AREA512_ARENA_BYTE_SIZE;
}

int
area512_metrics_arena_free_byte_size(void) {
  return AREA512_ARENA_BYTE_SIZE - s_recorded_arena_used_byte_size;
}
