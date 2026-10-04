#include "area512_hal.h"

#include "esp_app_format.h"
#include "esp_flash_partitions.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_system.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define COPY_BUFFER_SIZE 4096
#define ERASE_CHUNK_SIZE 0x10000
#define APP_HEADER_ERASE_SIZE 0x1000
#define APP_NAME_SIZE 32
#define MERGED_IMAGE_PARTITION_TABLE_OFFSET 0x8000
#define PROGRESS_TEXT_SIZE 48
#define PROGRESS_STEP_PERCENT 10

static void
write_console_line(const char *text) {
  area512_console_write(text, strlen(text));
  area512_console_write("\n", 1);
}

static void
write_progress_line(long done_byte_count, long total_byte_count) {
  char progress_text[PROGRESS_TEXT_SIZE];

  snprintf(
    progress_text,
    sizeof(progress_text),
    "%ld%%",
    done_byte_count * 100 / total_byte_count
  );

  write_console_line(progress_text);
}

static const esp_partition_t *
find_ota_partition(char *message, size_t message_size) {
  if (
    esp_ota_get_running_partition()->subtype !=
    ESP_PARTITION_SUBTYPE_APP_FACTORY
  ) {

    snprintf(message, message_size, "Not running from factory");
    return NULL;
  }

  const esp_partition_t *ota_partition =
    esp_partition_find_first(
      ESP_PARTITION_TYPE_APP,
      ESP_PARTITION_SUBTYPE_APP_OTA_0,
      NULL
    );

  if (ota_partition == NULL) {
    snprintf(message, message_size, "No ota_0 partition");
    return NULL;
  }

  return ota_partition;
}

static int
read_app_name(
  const esp_partition_t *ota_partition,
  char *name,
  size_t name_size
) {

  esp_app_desc_t app_description;

  if (
    esp_ota_get_partition_description(
      ota_partition,
      &app_description
    ) != ESP_OK
  ) {

    return -1;
  }

  snprintf(name, name_size, "%s", app_description.project_name);

  return 0;
}

static long
measure_file_byte_count(FILE *file) {
  if (fseek(file, 0, SEEK_END) != 0) {
    return -1;
  }

  long byte_count = ftell(file);

  if (fseek(file, 0, SEEK_SET) != 0) {
    return -1;
  }

  return byte_count;
}

static bool
has_partition_table_at_merged_offset(FILE *file, long image_byte_count) {
  uint16_t magic = 0;

  if (
    image_byte_count < MERGED_IMAGE_PARTITION_TABLE_OFFSET + (long)sizeof(magic)
  ) {

    return false;
  }

  bool magic_was_read =
    fseek(file, MERGED_IMAGE_PARTITION_TABLE_OFFSET, SEEK_SET) == 0 &&
    fread(&magic, sizeof(magic), 1, file) == 1;

  fseek(file, 0, SEEK_SET);

  return magic_was_read && magic == ESP_PARTITION_MAGIC;
}

static int
check_app_image_file(
  FILE *file,
  long image_byte_count,
  const esp_partition_t *ota_partition,
  char *message,
  size_t message_size
) {

  if (image_byte_count <= 0) {
    snprintf(message, message_size, "Empty file");
    return -1;
  }

  if ((size_t)image_byte_count > ota_partition->size) {
    snprintf(
      message,
      message_size,
      "Too large: %ld > %lu bytes",
      image_byte_count,
      (unsigned long)ota_partition->size
    );

    return -1;
  }

  int first_byte = fgetc(file);

  fseek(file, 0, SEEK_SET);

  if (first_byte != ESP_IMAGE_HEADER_MAGIC) {
    snprintf(message, message_size, "Not an app image");
    return -1;
  }

  if (has_partition_table_at_merged_offset(file, image_byte_count)) {
    snprintf(message, message_size, "Merged image not supported");
    return -1;
  }

  return 0;
}

static int
copy_file_to_ota_partition(
  FILE *file,
  long image_byte_count,
  const esp_partition_t *ota_partition,
  char *message,
  size_t message_size
) {

  uint8_t *copy_buffer = malloc(COPY_BUFFER_SIZE);

  if (copy_buffer == NULL) {
    snprintf(message, message_size, "Out of memory");
    return -1;
  }

  esp_ota_handle_t ota_handle;

  esp_err_t result =
    esp_ota_begin(
      ota_partition,
      OTA_WITH_SEQUENTIAL_WRITES,
      &ota_handle
    );

  if (result != ESP_OK) {
    free(copy_buffer);

    snprintf(
      message,
      message_size,
      "esp_ota_begin: %s",
      esp_err_to_name(result)
    );

    return -1;
  }

  long written_byte_count = 0;
  long next_progress_byte_count = 0;

  while (written_byte_count < image_byte_count) {
    size_t read_byte_count =
      fread(copy_buffer, 1, COPY_BUFFER_SIZE, file);

    if (read_byte_count == 0) {
      snprintf(message, message_size, "Read failed");
      result = ESP_FAIL;
      break;
    }

    result =
      esp_ota_write(
        ota_handle,
        copy_buffer,
        read_byte_count
      );

    if (result != ESP_OK) {
      snprintf(
        message,
        message_size,
        "esp_ota_write: %s",
        esp_err_to_name(result)
      );

      break;
    }

    written_byte_count += (long)read_byte_count;

    if (written_byte_count >= next_progress_byte_count) {
      write_progress_line(
        written_byte_count,
        image_byte_count
      );

      next_progress_byte_count +=
        image_byte_count * PROGRESS_STEP_PERCENT / 100;
    }
  }

  free(copy_buffer);

  if (result != ESP_OK) {
    esp_ota_abort(ota_handle);
    return -1;
  }

  result = esp_ota_end(ota_handle);

  if (result != ESP_OK) {
    snprintf(
      message,
      message_size,
      "esp_ota_end: %s",
      esp_err_to_name(result)
    );

    return -1;
  }

  return 0;
}

void
area512_restore_factory_boot_partition(void) {
  const esp_partition_t *running_partition = esp_ota_get_running_partition();

  if (running_partition->subtype != ESP_PARTITION_SUBTYPE_APP_FACTORY) {
    return;
  }

  if (esp_ota_get_boot_partition() == running_partition) {
    return;
  }

  esp_ota_set_boot_partition(running_partition);
}

int
area512_read_installed_app_name(char *name, size_t name_size) {
  const esp_partition_t *ota_partition =
    find_ota_partition(NULL, 0);

  if (ota_partition == NULL) {
    return -1;
  }

  return read_app_name(ota_partition, name, name_size);
}

int
area512_install_app_image(
  const char *path,
  char *message,
  size_t message_size
) {

  const esp_partition_t *ota_partition =
    find_ota_partition(message, message_size);

  if (ota_partition == NULL) {
    return -1;
  }

  char full_path[AREA512_PATH_MAX];

  if (
    area512_resolve_data_path(
      path,
      full_path,
      sizeof(full_path)
    ) != 0
  ) {

    snprintf(message, message_size, "Bad path");
    return -1;
  }

  FILE *file = fopen(full_path, "rb");

  if (file == NULL) {
    snprintf(message, message_size, "Cannot open");
    return -1;
  }

  long image_byte_count = measure_file_byte_count(file);

  if (
    check_app_image_file(
      file,
      image_byte_count,
      ota_partition,
      message,
      message_size
    ) != 0
  ) {

    fclose(file);
    return -1;
  }

  area512_console_reset();
  write_console_line("Installing to ota_0");

  int copy_result =
    copy_file_to_ota_partition(
      file,
      image_byte_count,
      ota_partition,
      message,
      message_size
    );

  fclose(file);

  if (copy_result != 0) {
    esp_partition_erase_range(
      ota_partition,
      0,
      APP_HEADER_ERASE_SIZE
    );

    return -1;
  }

  char name[APP_NAME_SIZE] = "";

  read_app_name(ota_partition, name, sizeof(name));
  snprintf(message, message_size, "Installed %s", name);

  return 0;
}

int
area512_launch_installed_app(char *message, size_t message_size) {
  const esp_partition_t *ota_partition =
    find_ota_partition(message, message_size);

  if (ota_partition == NULL) {
    return -1;
  }

  esp_err_t result = esp_ota_set_boot_partition(ota_partition);

  if (result != ESP_OK) {
    snprintf(
      message,
      message_size,
      "esp_ota_set_boot_partition: %s",
      esp_err_to_name(result)
    );
    return -1;
  }

  esp_restart();
}

int
area512_uninstall_app(char *message, size_t message_size) {
  const esp_partition_t *ota_partition =
    find_ota_partition(message, message_size);

  if (ota_partition == NULL) {
    return -1;
  }

  char name[APP_NAME_SIZE];

  if (read_app_name(ota_partition, name, sizeof(name)) != 0) {
    snprintf(message, message_size, "No app installed");
    return -1;
  }

  area512_console_reset();
  write_console_line("Erasing ota_0");

  long partition_byte_count = (long)ota_partition->size;
  long next_progress_byte_count = 0;

  for (
    long offset = 0;
    offset < partition_byte_count;
    offset += ERASE_CHUNK_SIZE
  ) {

    esp_err_t result =
      esp_partition_erase_range(
        ota_partition,
        offset,
        ERASE_CHUNK_SIZE
      );

    if (result != ESP_OK) {
      snprintf(
        message,
        message_size,
        "esp_partition_erase_range: %s",
        esp_err_to_name(result)
      );
      return -1;
    }

    long erased_byte_count = offset + ERASE_CHUNK_SIZE;

    if (erased_byte_count >= next_progress_byte_count) {
      write_progress_line(
        erased_byte_count,
        partition_byte_count
      );

      next_progress_byte_count +=
        partition_byte_count * PROGRESS_STEP_PERCENT / 100;
    }
  }

  snprintf(message, message_size, "Uninstalled %s", name);

  return 0;
}
