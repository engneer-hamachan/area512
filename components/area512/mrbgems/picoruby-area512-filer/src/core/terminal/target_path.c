#if defined(PICORB_VM_MRUBYC)

#include "area512_hal.h"
#include "core/terminal/terminal.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

void
expand_tilde(
  const char *input_path,
  char *expanded_path,
  int expanded_path_capacity
) {

  if (input_path[0] != '~' || (input_path[1] != 0 && input_path[1] != '/')) {
    strncpy(expanded_path, input_path, expanded_path_capacity - 1);
    expanded_path[expanded_path_capacity - 1] = 0;

    return;
  }

  snprintf(
    expanded_path,
    expanded_path_capacity,
    "%s%s",
    TERMINAL_HOME_DIRECTORY,
    input_path + 1
  );
}

static int
append_path_components(
  char *absolute_path,
  int *absolute_path_byte_count,
  int absolute_path_capacity,
  const char *path
) {

  const char *component;
  int component_byte_count;
  int last_slash_byte_offset;

  while (*path) {
    while (*path == '/')
      path++;

    component = path;

    while (*path && *path != '/')
      path++;

    component_byte_count = (int)(path - component);

    if (component_byte_count == 0)
      continue;

    if (component_byte_count == 1 && component[0] == '.')
      continue;

    if (
      component_byte_count == 2 &&
      component[0] == '.' &&
      component[1] == '.'
    ) {

      if (*absolute_path_byte_count == 0)
        return -1;

      last_slash_byte_offset = *absolute_path_byte_count - 1;

      while (absolute_path[last_slash_byte_offset] != '/')
        last_slash_byte_offset--;

      *absolute_path_byte_count = last_slash_byte_offset;
      absolute_path[last_slash_byte_offset] = 0;

      continue;
    }

    if (
      *absolute_path_byte_count + 1 + component_byte_count >
      absolute_path_capacity - 1
    )
      return -1;

    absolute_path[*absolute_path_byte_count] = '/';

    memcpy(
      absolute_path + *absolute_path_byte_count + 1,
      component,
      component_byte_count
    );

    *absolute_path_byte_count += 1 + component_byte_count;
    absolute_path[*absolute_path_byte_count] = 0;
  }

  return 0;
}

static int
stat_data_path(const char *absolute_path, struct stat *stat_buffer) {
  char resolved_path[AREA512_PATH_MAX];

  if (
    area512_resolve_data_path(
      absolute_path,
      resolved_path,
      sizeof resolved_path
    ) != 0
  )
    return -1;

  return stat(resolved_path, stat_buffer) == 0 ? 0 : -1;
}

int
resolve_target_path(
  const char *current_directory,
  const char *input_path,
  char *absolute_path,
  int absolute_path_capacity,
  int *target_is_directory
) {

  int absolute_path_byte_count = 0;
  struct stat stat_buffer;

  absolute_path[0] = 0;

  if (
    input_path[0] != '/' &&
    append_path_components(
      absolute_path,
      &absolute_path_byte_count,
      absolute_path_capacity,
      current_directory
    ) != 0
  )
    return TARGET_INVALID;

  if (
    append_path_components(
      absolute_path,
      &absolute_path_byte_count,
      absolute_path_capacity,
      input_path
    ) != 0
  )
    return TARGET_INVALID;

  if (absolute_path_byte_count == 0) {
    absolute_path[0] = '/';
    absolute_path[1] = 0;
  }

  if (stat_data_path(absolute_path, &stat_buffer) != 0)
    return TARGET_MISSING;

  *target_is_directory = S_ISDIR(stat_buffer.st_mode) ? 1 : 0;

  return TARGET_RESOLVED;
}

int
parent_path_is_directory(const char *absolute_path) {
  char parent_path[CURRENT_DIRECTORY_MAX];
  struct stat stat_buffer;
  int last_slash_byte_offset = (int)strlen(absolute_path) - 1;
  int parent_path_byte_count;

  while (
    last_slash_byte_offset > 0 &&
    absolute_path[last_slash_byte_offset] != '/'
  )
    last_slash_byte_offset--;

  parent_path_byte_count =
    last_slash_byte_offset > 0 ? last_slash_byte_offset : 1;

  if (parent_path_byte_count > (int)sizeof parent_path - 1)
    return 0;

  memcpy(parent_path, absolute_path, parent_path_byte_count);
  parent_path[parent_path_byte_count] = 0;

  if (stat_data_path(parent_path, &stat_buffer) != 0)
    return 0;

  return S_ISDIR(stat_buffer.st_mode) ? 1 : 0;
}

#endif
