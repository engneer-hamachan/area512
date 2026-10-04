#include "area512_hal.h"

#include "esp_rom_crc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hal/usb_serial_jtag_ll.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define REPLY_PREFIX "AREA512 "
#define READY_INTERVAL_MS 1000
#define DRAIN_SILENCE_MS 500
#define DRAIN_BUFFER_SIZE 64
#define COMMAND_LINE_SIZE (AREA512_PATH_MAX + 32)
#define FILE_BUFFER_SIZE 512
#define PROGRESS_TEXT_SIZE 48
#define ASCII_ESCAPE 0x1B

#define MKDIR_COMMAND_PREFIX "MKDIR "
#define FILE_COMMAND_PREFIX "FILE "
#define DATA_COMMAND_PREFIX "DATA "
#define END_COMMAND "END"
#define DONE_COMMAND "DONE"

#define LINE_READ 0
#define LINE_ESCAPE_PRESSED 1

#define CHUNK_WRITTEN 0
#define CHUNK_WRITE_FAILED 1
#define CHUNK_ESCAPE_PRESSED 2

#define COMMAND_EXECUTED 0
#define COMMAND_ESCAPE_PRESSED 1
#define COMMAND_DONE_RECEIVED 2

typedef struct {
  FILE *file;
  char path[AREA512_PATH_MAX];
  char full_path[AREA512_PATH_MAX];
  long byte_count;
  long received_byte_count;
  uint32_t crc32;
} ReceivingFile;

static volatile bool s_owns_rxfifo = false;

bool
area512_usb_receive_owns_rxfifo(void) {
  return s_owns_rxfifo;
}

static void
write_reply(const char *reply_text) {
  fputs(REPLY_PREFIX, stdout);
  fputs(reply_text, stdout);
  fputc('\n', stdout);
  fflush(stdout);

  usb_serial_jtag_ll_txfifo_flush();
}

static void
write_console_line(const char *text) {
  area512_console_write(text, strlen(text));
  area512_console_write("\n", 1);
}

static void
draw_progress_row(
  void *progress_row_sprite,
  const ReceivingFile *receiving_file
) {

  if (progress_row_sprite == NULL) {
    return;
  }

  char progress_text[PROGRESS_TEXT_SIZE];

  snprintf(
    progress_text,
    sizeof(progress_text),
    "%ld/%ld bytes",
    receiving_file->received_byte_count,
    receiving_file->byte_count
  );

  area512_sprite_fill(
    progress_row_sprite,
    area512_theme_background_color()
  );

  area512_sprite_text(
    progress_row_sprite,
    0,
    0,
    progress_text,
    area512_theme_text_color()
  );

  area512_sprite_push(
    progress_row_sprite,
    0,
    area512_console_cursor_row_index() * area512_console_row_height()
  );
}

static int
read_pending_escape_key_count(void) {
  int character;
  int escape_key_count = 0;

  while ((character = area512_console_getchar()) >= 0) {
    if (character == ASCII_ESCAPE) {
      escape_key_count++;
    }
  }

  return escape_key_count;
}

static int
read_command_line(char *command_line) {
  int command_line_byte_count = 0;
  uint8_t received_byte;

  TickType_t ready_sent_tick =
    xTaskGetTickCount() - pdMS_TO_TICKS(READY_INTERVAL_MS);

  for (;;) {
    if (usb_serial_jtag_ll_read_rxfifo(&received_byte, 1) == 1) {
      if (received_byte == '\n') {
        command_line[command_line_byte_count] = '\0';

        return LINE_READ;
      }

      if (command_line_byte_count < COMMAND_LINE_SIZE - 1) {
        command_line[command_line_byte_count] = (char)received_byte;
        command_line_byte_count++;
      }

      continue;
    }

    if (read_pending_escape_key_count() > 0) {
      return LINE_ESCAPE_PRESSED;
    }

    if (
      command_line_byte_count == 0 &&
      xTaskGetTickCount() - ready_sent_tick >= pdMS_TO_TICKS(READY_INTERVAL_MS)
    ) {

      write_reply("READY");

      ready_sent_tick = xTaskGetTickCount();
    }

    vTaskDelay(1);
  }
}

static int
receive_chunk(ReceivingFile *receiving_file, long chunk_byte_count) {
  uint8_t file_buffer[FILE_BUFFER_SIZE];
  long remaining_byte_count = chunk_byte_count;
  bool write_failed = false;

  while (remaining_byte_count > 0) {
    uint32_t requested_byte_count =
      remaining_byte_count < FILE_BUFFER_SIZE
        ? (uint32_t)remaining_byte_count
        : FILE_BUFFER_SIZE;

    uint32_t read_byte_count =
      usb_serial_jtag_ll_read_rxfifo(
        file_buffer,
        requested_byte_count
      );

    if (read_byte_count == 0) {
      if (read_pending_escape_key_count() > 0) {
        return CHUNK_ESCAPE_PRESSED;
      }

      vTaskDelay(1);

      continue;
    }

    if (
      !write_failed &&
      fwrite(
        file_buffer,
        1,
        read_byte_count,
        receiving_file->file
     ) != read_byte_count) {

      write_failed = true;
    }

    remaining_byte_count -= read_byte_count;
  }

  if (write_failed) {
    return CHUNK_WRITE_FAILED;
  }

  receiving_file->received_byte_count += chunk_byte_count;

  return CHUNK_WRITTEN;
}

static int
compute_file_crc32(const char *full_path, uint32_t *crc32) {
  FILE *file = fopen(full_path, "rb");

  if (file == NULL) {
    return -1;
  }

  uint8_t file_buffer[FILE_BUFFER_SIZE];
  size_t read_byte_count;
  uint32_t computed_crc32 = 0;

  while (
      (
        read_byte_count =
          fread(file_buffer, 1, sizeof(file_buffer), file)
      ) > 0
    ) {

    computed_crc32 =
      esp_rom_crc32_le(
        computed_crc32,
        file_buffer,
        read_byte_count
      );
  }

  bool read_failed = ferror(file) != 0;

  fclose(file);

  if (read_failed) {
    return -1;
  }

  *crc32 = computed_crc32;

  return 0;
}

static void
close_and_delete_receiving_file(ReceivingFile *receiving_file) {
  fclose(receiving_file->file);
  unlink(receiving_file->full_path);

  receiving_file->file = NULL;
}

static void
start_receiving_file(
  ReceivingFile *receiving_file,
  char *file_arguments,
  void *progress_row_sprite
) {

  char *crc32_text = strrchr(file_arguments, ' ');

  if (crc32_text == NULL) {
    write_reply("NG unknown command");

    return;
  }

  *crc32_text = '\0';
  crc32_text++;

  char *byte_count_text = strrchr(file_arguments, ' ');

  if (byte_count_text == NULL) {
    write_reply("NG unknown command");

    return;
  }

  *byte_count_text = '\0';
  byte_count_text++;

  if (strlen(file_arguments) >= sizeof(receiving_file->path)) {
    write_console_line("NG bad path");
    write_reply("NG bad path");

    return;
  }

  strcpy(receiving_file->path, file_arguments);

  write_console_line(receiving_file->path);

  if (
    area512_resolve_data_path(
      receiving_file->path,
      receiving_file->full_path,
      sizeof(receiving_file->full_path)
    ) != 0 ||
    strcmp(receiving_file->full_path, AREA512_DATA_ROOT) == 0
  ) {

    write_console_line("NG bad path");
    write_reply("NG bad path");

    return;
  }

  if (
    !area512_ensure_parent_directories(receiving_file->full_path)
  ) {

    write_console_line("NG mkdir failed");
    write_reply("NG mkdir failed");

    return;
  }

  receiving_file->file = fopen(receiving_file->full_path, "wb");

  if (receiving_file->file == NULL) {
    write_console_line("NG open failed");
    write_reply("NG open failed");

    return;
  }

  receiving_file->byte_count =
    strtol(byte_count_text, NULL, 10);

  receiving_file->received_byte_count = 0;

  receiving_file->crc32 =
    (uint32_t)strtoul(crc32_text, NULL, 16);

  draw_progress_row(progress_row_sprite, receiving_file);

  write_reply("OK");
}

static void
finish_receiving_file(ReceivingFile *receiving_file) {
  uint32_t computed_crc32;

  bool closed = fclose(receiving_file->file) == 0;

  receiving_file->file = NULL;

  if (!closed) {
    unlink(receiving_file->full_path);

    write_console_line("NG write failed");
    write_reply("NG write failed");

    return;
  }

  if (
    compute_file_crc32(
      receiving_file->full_path,
      &computed_crc32
    ) != 0 ||
    computed_crc32 != receiving_file->crc32
  ) {

    unlink(receiving_file->full_path);

    write_console_line("NG crc mismatch");
    write_reply("NG crc mismatch");

    return;
  }

  write_console_line("OK");
  write_reply("OK");
}

static void
make_directory(const char *path) {
  char full_path[AREA512_PATH_MAX];

  write_console_line(path);

  if (
    area512_resolve_data_path(
      path,
      full_path,
      sizeof(full_path)
    ) != 0 ||
    strcmp(full_path, AREA512_DATA_ROOT) == 0
  ) {

    write_console_line("NG bad path");
    write_reply("NG bad path");

    return;
  }

  if (
    !area512_ensure_parent_directories(full_path) ||
    !area512_ensure_directory(full_path)
  ) {

    write_console_line("NG mkdir failed");
    write_reply("NG mkdir failed");

    return;
  }

  write_console_line("OK");
  write_reply("OK");
}

static int
execute_command_line(
  ReceivingFile *receiving_file,
  char *command_line,
  void *progress_row_sprite
) {

  if (
    receiving_file->file != NULL &&
    strncmp(
      command_line,
      DATA_COMMAND_PREFIX,
      strlen(DATA_COMMAND_PREFIX)
    ) == 0
  ) {

    long chunk_byte_count =
      strtol(
        command_line + strlen(DATA_COMMAND_PREFIX),
        NULL,
        10
      );

    int chunk_result = receive_chunk(receiving_file, chunk_byte_count);

    if (chunk_result == CHUNK_ESCAPE_PRESSED) {
      return COMMAND_ESCAPE_PRESSED;
    }

    if (chunk_result == CHUNK_WRITE_FAILED) {
      close_and_delete_receiving_file(receiving_file);

      write_console_line("NG write failed");
      write_reply("NG write failed");

      return COMMAND_EXECUTED;
    }

    draw_progress_row(progress_row_sprite, receiving_file);

    write_reply("OK");

    return COMMAND_EXECUTED;
  }

  if (receiving_file->file != NULL && strcmp(command_line, END_COMMAND) == 0) {
    finish_receiving_file(receiving_file);

    return COMMAND_EXECUTED;
  }

  if (receiving_file->file != NULL) {
    close_and_delete_receiving_file(receiving_file);
  }

  if (
    strncmp(
      command_line,
      MKDIR_COMMAND_PREFIX,
      strlen(MKDIR_COMMAND_PREFIX)
    ) == 0
  ) {

    make_directory(command_line + strlen(MKDIR_COMMAND_PREFIX));

  } else if (
    strncmp(
      command_line,
      FILE_COMMAND_PREFIX,
      strlen(FILE_COMMAND_PREFIX)
    ) == 0
  ) {

    start_receiving_file(
      receiving_file,
      command_line + strlen(FILE_COMMAND_PREFIX),
      progress_row_sprite
    );

  } else if (strcmp(command_line, DONE_COMMAND) == 0) {
    write_reply("OK");

    return COMMAND_DONE_RECEIVED;
  } else {
    write_reply("NG unknown command");
  }

  return COMMAND_EXECUTED;
}

static void
drain_rxfifo(void) {
  uint8_t discarded_bytes[DRAIN_BUFFER_SIZE];
  TickType_t last_received_tick = xTaskGetTickCount();

  while (
    xTaskGetTickCount() - last_received_tick < pdMS_TO_TICKS(DRAIN_SILENCE_MS)
  ) {

    if (
      usb_serial_jtag_ll_read_rxfifo(
        discarded_bytes,
        sizeof(discarded_bytes)
      ) > 0
    ) {

      last_received_tick = xTaskGetTickCount();

    } else {
      vTaskDelay(1);
    }
  }
}

void
area512_usb_receive_files(void) {
  ReceivingFile receiving_file;
  char command_line[COMMAND_LINE_SIZE];

  memset(&receiving_file, 0, sizeof(receiving_file));

  s_owns_rxfifo = true;

  area512_console_reset();

  void *progress_row_sprite =
    area512_sprite_new_with_font_size(
      area512_gfx_width(),
      area512_console_row_height(),
      area512_console_font_size()
    );

  write_console_line("Receiving over USB. Esc to exit");

  for (;;) {
    if (read_command_line(command_line) == LINE_ESCAPE_PRESSED) {
      break;
    }

    if (
      execute_command_line(
        &receiving_file,
        command_line,
        progress_row_sprite
      ) != COMMAND_EXECUTED
    ) {

      break;
    }
  }

  if (receiving_file.file != NULL) {
    close_and_delete_receiving_file(&receiving_file);
  }

  drain_rxfifo();

  area512_sprite_delete(progress_row_sprite);

  s_owns_rxfifo = false;
}
