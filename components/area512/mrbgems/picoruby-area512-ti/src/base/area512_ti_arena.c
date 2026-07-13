#include "area512_ti_arena.h"
#include <stdint.h>

static uint8_t arena_bytes[TI_ARENA_SIZE];
static size_t arena_offset;
static int arena_overflowed;

void
ti_reset_arena(void) {
  arena_offset = 0;
  arena_overflowed = 0;
}

void *
ti_allocate_from_arena(size_t size) {
  size_t aligned_size = (size + 7U) & ~(size_t)7U;

  if (aligned_size > TI_ARENA_SIZE - arena_offset) {
    arena_overflowed = 1;
    return NULL;
  }

  void *allocation = &arena_bytes[arena_offset];
  arena_offset += aligned_size;

  return allocation;
}

int
ti_did_arena_overflow(void) {
  return arena_overflowed;
}
