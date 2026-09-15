#include "area512_hal.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#define TAR_BLOCK_SIZE 512
#define TAR_NAME_OFFSET 0
#define TAR_NAME_SIZE 100
#define TAR_SIZE_OFFSET 124
#define TAR_SIZE_SIZE 12
#define TAR_TYPE_OFFSET 156

// Symbols come from target_add_binary_data(area512_seed.tar) in
// components/area512_hal/CMakeLists.txt.
extern const uint8_t
  area512_seed_tar_start[] asm("_binary_area512_seed_tar_start");
extern const uint8_t area512_seed_tar_end[] asm("_binary_area512_seed_tar_end");

static const char *const s_seed_directories[] =
  {"home", "lib", "bin", "etc", "data"};

#define SEED_DIRECTORY_COUNT                                                   \
  (sizeof(s_seed_directories) / sizeof(s_seed_directories[0]))

static bool
ensure_directory(const char *path) {
  return mkdir(path, 0777) == 0 || errno == EEXIST;
}

static size_t
parse_octal_field(const uint8_t *field, size_t field_size) {
  size_t value = 0;
  size_t i = 0;

  while (i < field_size && (field[i] == ' ' || field[i] == '0')) {
    i++;
  }

  while (i < field_size && field[i] >= '0' && field[i] <= '7') {
    value = value * 8 + (size_t)(field[i] - '0');
    i++;
  }

  return value;
}

typedef struct {
  char name[TAR_NAME_SIZE + 1];
  size_t size;
  uint8_t type;
  const uint8_t *data;
} TarEntry;

static const uint8_t *
read_tar_entry(const uint8_t *cursor, TarEntry *entry) {
  if (cursor + TAR_BLOCK_SIZE > area512_seed_tar_end ||
      cursor[TAR_NAME_OFFSET] == '\0') {

    return NULL;
  }

  memcpy(entry->name, cursor + TAR_NAME_OFFSET, TAR_NAME_SIZE);

  entry->name[TAR_NAME_SIZE] = '\0';
  entry->size = parse_octal_field(cursor + TAR_SIZE_OFFSET, TAR_SIZE_SIZE);
  entry->type = cursor[TAR_TYPE_OFFSET];
  entry->data = cursor + TAR_BLOCK_SIZE;

  size_t padded_size =
    (entry->size + TAR_BLOCK_SIZE - 1) & ~(size_t)(TAR_BLOCK_SIZE - 1);

  return entry->data + padded_size;
}

static bool
is_regular_file_entry(const TarEntry *entry) {
  return entry->type == '0' || entry->type == '\0';
}

static int
find_seed_directory_index(const char *entry_name) {
  for (size_t i = 0; i < SEED_DIRECTORY_COUNT; i++) {
    size_t name_length = strlen(s_seed_directories[i]);

    if (strncmp(entry_name, s_seed_directories[i], name_length) == 0 &&
        (entry_name[name_length] == '/' || entry_name[name_length] == '\0')) {

      return (int)i;
    }
  }

  return -1;
}

static bool
ensure_parent_directories(char *full_path) {
  for (char *cursor = full_path + strlen(AREA512_DATA_ROOT) + 1; *cursor;
       cursor++) {

    if (*cursor != '/') {
      continue;
    }

    *cursor = '\0';

    bool created = ensure_directory(full_path);

    *cursor = '/';

    if (!created) {
      return false;
    }
  }

  return true;
}

static bool
write_seed_file(const char *relative_path, const uint8_t *data, size_t size) {

  char full_path[sizeof(AREA512_DATA_ROOT) + TAR_NAME_SIZE + 1];

  snprintf(
    full_path,
    sizeof(full_path),
    "%s/%s",
    AREA512_DATA_ROOT,
    relative_path
  );

  if (!ensure_parent_directories(full_path)) {
    return false;
  }

  FILE *file = fopen(full_path, "wb");

  if (file == NULL) {
    return false;
  }

  size_t written = size > 0 ? fwrite(data, 1, size, file) : 0;

  bool closed = fclose(file) == 0;

  return written == size && closed;
}

int
area512_seed_restore(void) {
  if (!ensure_directory(AREA512_DATA_ROOT)) {
    return -1;
  }

  bool restore[SEED_DIRECTORY_COUNT];

  for (size_t i = 0; i < SEED_DIRECTORY_COUNT; i++) {
    char directory_path[sizeof(AREA512_DATA_ROOT) + TAR_NAME_SIZE + 1];

    snprintf(
      directory_path,
      sizeof(directory_path),
      "%s/%s",
      AREA512_DATA_ROOT,
      s_seed_directories[i]
    );

    struct stat stat_buffer;

    if (stat(directory_path, &stat_buffer) == 0) {
      restore[i] = false;
      continue;
    }

    if (mkdir(directory_path, 0777) != 0) {
      return -1;
    }

    restore[i] = true;
  }

  const uint8_t *cursor = area512_seed_tar_start;
  const uint8_t *next_cursor;
  TarEntry entry;

  while ((next_cursor = read_tar_entry(cursor, &entry)) != NULL) {
    int directory_index = find_seed_directory_index(entry.name);
    bool wanted = directory_index >= 0 && restore[directory_index];

    if (wanted && is_regular_file_entry(&entry)) {
      if (!write_seed_file(entry.name, entry.data, entry.size)) {
        return -1;
      }

    } else if (wanted && entry.type == '5') {
      size_t name_length = strlen(entry.name);

      if (name_length > 0 && entry.name[name_length - 1] == '/') {
        entry.name[name_length - 1] = '\0';
      }

      char directory_path[sizeof(AREA512_DATA_ROOT) + TAR_NAME_SIZE + 1];

      snprintf(
        directory_path,
        sizeof(directory_path),
        "%s/%s",
        AREA512_DATA_ROOT,
        entry.name
      );

      if (!ensure_parent_directories(directory_path) ||
          !ensure_directory(directory_path)) {

        return -1;
      }
    }

    cursor = next_cursor;
  }

  return 0;
}

const uint8_t *
area512_seed_find_file(const char *path, size_t *size) {
  while (*path == '/') {
    path++;
  }

  const uint8_t *cursor = area512_seed_tar_start;
  const uint8_t *next_cursor;
  TarEntry entry;

  while ((next_cursor = read_tar_entry(cursor, &entry)) != NULL) {
    if (is_regular_file_entry(&entry) && strcmp(entry.name, path) == 0 &&
        entry.data + entry.size <= area512_seed_tar_end) {

      *size = entry.size;

      return entry.data;
    }

    cursor = next_cursor;
  }

  return NULL;
}
