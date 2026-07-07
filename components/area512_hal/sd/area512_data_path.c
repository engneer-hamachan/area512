#include "area512_hal.h"

#include <string.h>

static int
has_parent_reference(const char *path) {
  while (*path) {
    const char *component = path;

    while (*path && *path != '/')
      path++;

    if (path - component == 2 && component[0] == '.' && component[1] == '.')
      return 1;

    while (*path == '/')
      path++;
  }

  return 0;
}

int
area512_resolve_data_path(const char *path, char *buffer, size_t buffer_size) {
  while (*path == '/')
    path++;

  // path traversal guard
  if (has_parent_reference(path))
    return -1;

  size_t root_length = strlen(AREA512_DATA_ROOT);
  size_t path_length = strlen(path);
  size_t full_length = root_length + (path_length > 0 ? 1 + path_length : 0);

  if (full_length + 1 > buffer_size)
    return -1;

  memcpy(buffer, AREA512_DATA_ROOT, root_length);

  if (path_length > 0) {
    buffer[root_length] = '/';
    memcpy(buffer + root_length + 1, path, path_length + 1);
  } else {
    buffer[root_length] = '\0';
  }

  return 0;
}
