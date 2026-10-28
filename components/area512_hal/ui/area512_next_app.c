#include "area512_hal.h"

#include <string.h>

static char s_next_app_path[AREA512_PATH_MAX];

int
area512_set_next_app(const char *path) {
  size_t path_length = strlen(path);

  if (path_length + 1 > sizeof(s_next_app_path))
    return -1;

  memcpy(s_next_app_path, path, path_length + 1);

  return 0;
}

int
area512_take_next_app(char *path, size_t path_size) {
  size_t path_length = strlen(s_next_app_path);
  int is_taken = path_length > 0 && path_length + 1 <= path_size;

  if (is_taken)
    memcpy(path, s_next_app_path, path_length + 1);

  s_next_app_path[0] = '\0';

  return is_taken;
}

void
area512_cancel_next_app(void) {
  s_next_app_path[0] = '\0';
}
