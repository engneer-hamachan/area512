// VM heap stats for the console status line. Symbols are forward-declared
// to avoid depending on picoruby internal headers.

#include <stddef.h>

struct MRBC_ALLOC_STATISTICS {
  unsigned int total;
  unsigned int used;
  unsigned int free;
  unsigned int fragmentation;
};

void mrbc_alloc_statistics(struct MRBC_ALLOC_STATISTICS *stat);

size_t
picoruby_esp32_vm_heap_free_size(void) {
  struct MRBC_ALLOC_STATISTICS stat;

  mrbc_alloc_statistics(&stat);

  return (size_t)stat.free;
}

// Total VM heap (denominator for usage bars).
size_t
picoruby_esp32_vm_heap_total_size(void) {
  struct MRBC_ALLOC_STATISTICS stat;

  mrbc_alloc_statistics(&stat);

  return (size_t)stat.total;
}
