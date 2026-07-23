#include "port/area512_editor_file.h"

#include "area512_hal.h"
#include "core/text/string.h"

#include <stdio.h>
#include <string.h>

static int
resolve_edit_path(
  char *destination,
  int destination_size,
  const char *path,
  int path_byte_length
) {

  char ruby_path[AREA512_PATH_MAX];

  int byte_length = path_byte_length;
  if (byte_length > (int)sizeof(ruby_path) - 1)
    byte_length = (int)sizeof(ruby_path) - 1;

  if (byte_length < 0)
    byte_length = 0;

  memcpy(ruby_path, path, byte_length);

  ruby_path[byte_length] = '\0';

  return area512_resolve_data_path(
           ruby_path,
           destination,
           (size_t)destination_size
         ) == 0;
}

int
load_edit_file(const char *path, int path_byte_length, VimString *content) {
  char path_buffer[AREA512_PATH_MAX];

  if (!resolve_edit_path(
        path_buffer,
        sizeof(path_buffer),
        path,
        path_byte_length
      ))
    return 0;

  FILE *file = fopen(path_buffer, "rb");

  if (file == NULL)
    return 0;

  char read_buffer[256];
  size_t read_size;

  while ((read_size = fread(read_buffer, 1, sizeof(read_buffer), file)) > 0) {
    vim_string_append(content, read_buffer, (int)read_size);
  }

  fclose(file);

  return 1;
}

int
save_edit_file(Vim *core) {
  if (core->filepath.byte_length <= 0)
    return 0;

  char path_buffer[AREA512_PATH_MAX];

  if (!resolve_edit_path(
        path_buffer,
        sizeof(path_buffer),
        core->filepath.bytes,
        core->filepath.byte_length
      ))
    return 0;

  VimString content;
  vim_string_init(&content);
  vim_write_content(core, &content);

  int saved = 0;

  FILE *file = fopen(path_buffer, "wb");

  if (file != NULL) {
    size_t written = 0;

    if (content.byte_length > 0)
      written = fwrite(content.bytes, 1, (size_t)content.byte_length, file);

    saved = written == (size_t)content.byte_length;

    if (fclose(file) != 0)
      saved = 0;
  }

  vim_string_free(&content);

  return saved;
}
