#if defined(PICORB_VM_MRUBYC)

#include "core/filer.h"

#include <string.h>


void
fit_string(
  char *destination,
  int destination_size,
  const char *source,
  int width
) {

  int length = (int)strlen(source);

  if (width < 1)
    width = 1;

  if (width > destination_size - 1)
    width = destination_size - 1;

  if (length <= width) {
    memcpy(destination, source, length);
    destination[length] = 0;
  } else {
    memcpy(destination, source, width - 1);
    destination[width - 1] = '~';
    destination[width] = 0;
  }
}

#endif
