#include "port/area512_editor_file.h"

#include "core/text/string.h"

#include <littlefs.h>
#include <string.h>

static void
copy_edit_path_c_string(
  char *destination,
  int destination_size,
  const char *path,
  int path_byte_length
) {

  int byte_length = path_byte_length;
  if (byte_length > destination_size - 1)
    byte_length = destination_size - 1;

  if (byte_length < 0)
    byte_length = 0;

  memcpy(destination, path, byte_length);

  destination[byte_length] = '\0';
}

int
load_edit_file(const char *path, int path_byte_length, VimString *content) {
  lfs_t *lfs = littlefs_get_lfs();

  if (!lfs)
    return 0;

  char path_buffer[LFS_NAME_MAX + 1];

  copy_edit_path_c_string(
    path_buffer,
    sizeof(path_buffer),
    path,
    path_byte_length
  );

  lfs_file_t file;
  if (lfs_file_open(lfs, &file, path_buffer, LFS_O_RDONLY) < 0)
    return 0;

  char read_buffer[256];
  lfs_ssize_t read_size;

  while ((read_size =
            lfs_file_read(lfs, &file, read_buffer, sizeof(read_buffer))) > 0) {
    vim_string_append(content, read_buffer, (int)read_size);
  }

  lfs_file_close(lfs, &file);

  return 1;
}

int
save_edit_file(Vim *core) {
  if (core->filepath.byte_length <= 0)
    return 0;

  lfs_t *lfs = littlefs_get_lfs();

  if (!lfs)
    return 0;

  char path_buffer[LFS_NAME_MAX + 1];

  copy_edit_path_c_string(
    path_buffer,
    sizeof(path_buffer),
    core->filepath.bytes,
    core->filepath.byte_length
  );

  VimString content;
  vim_string_init(&content);
  vim_write_content(core, &content);

  lfs_file_t file;

  int saved = 0;

  if (
        lfs_file_open(
          lfs,
          &file,
          path_buffer,
          LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC
        ) >= 0
      ) {

    int offset = 0;
    saved = 1;

    while (offset < content.byte_length) {
      lfs_ssize_t written =
        lfs_file_write(
          lfs,
          &file,
          content.bytes + offset,
          content.byte_length - offset
        );

      if (written < 0) {
        saved = 0;
        break;
      }

      offset += (int)written;
    }

    lfs_file_close(lfs, &file);
  }

  vim_string_free(&content);

  return saved;
}
