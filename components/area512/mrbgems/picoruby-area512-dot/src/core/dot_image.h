#pragma once

#include <stdint.h>

#define DOT_IMAGE_MAGIC "A5DI"
#define DOT_IMAGE_MAGIC_BYTE_LENGTH 4
#define DOT_IMAGE_VERSION 1
#define DOT_IMAGE_HEADER_BYTE_LENGTH 8
#define DOT_IMAGE_PALETTE_COLOR_COUNT_MAX 16
#define DOT_IMAGE_TRANSPARENT_COLOR_INDEX 0

typedef enum {
  DOT_IMAGE_STATUS_OK,
  DOT_IMAGE_STATUS_OPEN_FAILED,
  DOT_IMAGE_STATUS_EMPTY_FILE,
  DOT_IMAGE_STATUS_MAGIC_MISMATCH,
  DOT_IMAGE_STATUS_VERSION_UNSUPPORTED,
  DOT_IMAGE_STATUS_PALETTE_COLOR_COUNT_INVALID,
  DOT_IMAGE_STATUS_CONTENT_BYTE_LENGTH_SHORT,
  DOT_IMAGE_STATUS_OUT_OF_MEMORY,
  DOT_IMAGE_STATUS_WRITE_FAILED
} DotImageStatus;

typedef struct {
  int width;
  int height;
  int palette_color_count;
  int painted_left;
  int painted_top;
  int painted_right;
  int painted_bottom;
  int hitbox_left;
  int hitbox_top;
  int hitbox_right;
  int hitbox_bottom;
  uint16_t palette_rgb565[DOT_IMAGE_PALETTE_COLOR_COUNT_MAX];
  uint8_t pixel_bytes[];
} DotImage;

DotImageStatus load_dot_image_file(
  const char *path,
  int path_byte_length,
  DotImage **loaded_image
);

DotImageStatus
create_blank_dot_image(int width, int height, DotImage **created_image);

void free_dot_image(DotImage *image);

DotImageStatus save_dot_image_file(
  const DotImage *image,
  const char *path,
  int path_byte_length
);

int fetch_dot_image_color_index(const DotImage *image, int dot_x, int dot_y);

void set_dot_image_color_index(
  DotImage *image,
  int dot_x,
  int dot_y,
  int color_index
);

int has_dot_image_hitbox(const DotImage *image, int dot_x, int dot_y);

void set_dot_image_hitbox(
  DotImage *image,
  int dot_x,
  int dot_y,
  int has_hitbox
);

void push_dot_image_to_sprite(
  const DotImage *image,
  void *sprite,
  int screen_x,
  int screen_y
);

int has_dot_image_overlap(
  const DotImage *first_image,
  int first_screen_x,
  int first_screen_y,
  const DotImage *second_image,
  int second_screen_x,
  int second_screen_y,
  int use_hitbox
);

uint32_t convert_rgb565_to_rgb888(uint16_t rgb565);
