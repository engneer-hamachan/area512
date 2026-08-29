#if defined(PICORB_VM_MRUBYC)

#include "area512_hal.h"
#include "core/dot_image.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const uint16_t
  DEFAULT_PALETTE_RGB565[DOT_IMAGE_PALETTE_COLOR_COUNT_MAX] = {
    0x0000,
    0xFFFF,
    0xFAA8,
    0xFE66,
    0x3FE0,
    0x367F,
    0xC61F,
    0xFB3F,
    0x0000,
    0x8410,
    0xFC40,
    0x0400,
    0x0010,
    0x8010,
    0x8A22,
    0xC618
  };

static int
calculate_pixel_byte_length(int width, int height) {
  return (width / 2) * height;
}

static int
calculate_hitbox_row_byte_length(int width) {
  return (width + 7) / 8;
}

static int
calculate_hitbox_byte_length(int width, int height) {
  return calculate_hitbox_row_byte_length(width) * height;
}

static int
calculate_pixel_and_hitbox_byte_length(int width, int height) {
  return calculate_pixel_byte_length(width, height) +
         calculate_hitbox_byte_length(width, height);
}

static int
calculate_hitbox_byte_index(const DotImage *image, int dot_x, int dot_y) {
  return dot_y * calculate_hitbox_row_byte_length(image->width) + dot_x / 8;
}

static int
resolve_dot_image_path(
  const char *path,
  int path_byte_length,
  char *resolved_path,
  int resolved_path_size
) {

  char ruby_path[AREA512_PATH_MAX];
  int byte_length = path_byte_length;

  if (byte_length > (int)sizeof(ruby_path) - 1)
    byte_length = (int)sizeof(ruby_path) - 1;

  if (byte_length < 0)
    byte_length = 0;

  memcpy(ruby_path, path, (size_t)byte_length);
  ruby_path[byte_length] = '\0';

  return area512_resolve_data_path(
           ruby_path,
           resolved_path,
           (size_t)resolved_path_size
         ) == 0;
}

static void
calculate_painted_and_hitbox_bounds(DotImage *image) {
  image->painted_left = image->width;
  image->painted_top = image->height;
  image->painted_right = -1;
  image->painted_bottom = -1;
  image->hitbox_left = image->width;
  image->hitbox_top = image->height;
  image->hitbox_right = -1;
  image->hitbox_bottom = -1;

  for (int dot_y = 0; dot_y < image->height; dot_y++) {
    for (int dot_x = 0; dot_x < image->width; dot_x++) {
      if (
        fetch_dot_image_color_index(image, dot_x, dot_y) !=
        DOT_IMAGE_TRANSPARENT_COLOR_INDEX
      ) {

        if (dot_x < image->painted_left)
          image->painted_left = dot_x;

        if (dot_x > image->painted_right)
          image->painted_right = dot_x;

        if (dot_y < image->painted_top)
          image->painted_top = dot_y;

        if (dot_y > image->painted_bottom)
          image->painted_bottom = dot_y;
      }

      if (!has_dot_image_hitbox(image, dot_x, dot_y))
        continue;

      if (dot_x < image->hitbox_left)
        image->hitbox_left = dot_x;

      if (dot_x > image->hitbox_right)
        image->hitbox_right = dot_x;

      if (dot_y < image->hitbox_top)
        image->hitbox_top = dot_y;

      if (dot_y > image->hitbox_bottom)
        image->hitbox_bottom = dot_y;
    }
  }

  if (image->painted_right < 0) {
    image->painted_left = 0;
    image->painted_top = 0;
  }

  if (image->hitbox_right < 0) {
    image->hitbox_left = 0;
    image->hitbox_top = 0;
  }
}

DotImageStatus
load_dot_image_file(
  const char *path,
  int path_byte_length,
  DotImage **loaded_image
) {

  char resolved_path[AREA512_PATH_MAX];

  if (
    !resolve_dot_image_path(
      path,
      path_byte_length,
      resolved_path,
      (int)sizeof resolved_path
    )
  )
    return DOT_IMAGE_STATUS_OPEN_FAILED;

  FILE *file = fopen(resolved_path, "rb");

  if (file == NULL)
    return DOT_IMAGE_STATUS_OPEN_FAILED;

  uint8_t header_bytes[DOT_IMAGE_HEADER_BYTE_LENGTH];
  size_t read_byte_count = fread(header_bytes, 1, sizeof header_bytes, file);

  if (read_byte_count == 0) {
    fclose(file);
    return DOT_IMAGE_STATUS_EMPTY_FILE;
  }

  if (
    read_byte_count < sizeof header_bytes ||
    memcmp(header_bytes, DOT_IMAGE_MAGIC, DOT_IMAGE_MAGIC_BYTE_LENGTH) != 0
  ) {

    fclose(file);
    return DOT_IMAGE_STATUS_MAGIC_MISMATCH;
  }

  if (header_bytes[4] != DOT_IMAGE_VERSION) {
    fclose(file);
    return DOT_IMAGE_STATUS_VERSION_UNSUPPORTED;
  }

  int width = header_bytes[5];
  int height = header_bytes[6];
  int palette_color_count = header_bytes[7];

  if (
    palette_color_count < 1 ||
    palette_color_count > DOT_IMAGE_PALETTE_COLOR_COUNT_MAX
  ) {

    fclose(file);
    return DOT_IMAGE_STATUS_PALETTE_COLOR_COUNT_INVALID;
  }

  int pixel_byte_length = calculate_pixel_byte_length(width, height);
  int hitbox_byte_length = calculate_hitbox_byte_length(width, height);

  if (pixel_byte_length <= 0) {
    fclose(file);
    return DOT_IMAGE_STATUS_CONTENT_BYTE_LENGTH_SHORT;
  }

  DotImage *image =
    (DotImage *)malloc(
      sizeof(DotImage) + (size_t)(pixel_byte_length + hitbox_byte_length)
    );

  if (image == NULL) {
    fclose(file);
    return DOT_IMAGE_STATUS_OUT_OF_MEMORY;
  }

  image->width = width;
  image->height = height;
  image->palette_color_count = palette_color_count;

  memset(image->palette_rgb565, 0, sizeof image->palette_rgb565);

  for (
    int color_index = 0;
    color_index < palette_color_count;
    color_index++
  ) {

    uint8_t palette_bytes[2];

    if (
      fread(palette_bytes, 1, sizeof palette_bytes, file) !=
      sizeof palette_bytes
    ) {

      free(image);
      fclose(file);

      return DOT_IMAGE_STATUS_CONTENT_BYTE_LENGTH_SHORT;
    }

    image->palette_rgb565[color_index] =
      (uint16_t)(palette_bytes[0] | (palette_bytes[1] << 8));
  }

  read_byte_count =
    fread(image->pixel_bytes, 1, (size_t)pixel_byte_length, file);

  if (read_byte_count != (size_t)pixel_byte_length) {
    free(image);
    fclose(file);

    return DOT_IMAGE_STATUS_CONTENT_BYTE_LENGTH_SHORT;
  }

  uint8_t *hitbox_bytes = image->pixel_bytes + pixel_byte_length;

  read_byte_count = fread(hitbox_bytes, 1, (size_t)hitbox_byte_length, file);

  fclose(file);

  if (read_byte_count != (size_t)hitbox_byte_length)
    memset(hitbox_bytes, 0, (size_t)hitbox_byte_length);

  calculate_painted_and_hitbox_bounds(image);

  *loaded_image = image;

  return DOT_IMAGE_STATUS_OK;
}

DotImageStatus
create_blank_dot_image(int width, int height, DotImage **created_image) {
  int pixel_byte_length = calculate_pixel_byte_length(width, height);

  if (pixel_byte_length <= 0)
    return DOT_IMAGE_STATUS_CONTENT_BYTE_LENGTH_SHORT;

  int pixel_and_hitbox_byte_length =
    calculate_pixel_and_hitbox_byte_length(width, height);

  DotImage *image = (DotImage *)malloc(
    sizeof(DotImage) + (size_t)pixel_and_hitbox_byte_length
  );

  if (image == NULL)
    return DOT_IMAGE_STATUS_OUT_OF_MEMORY;

  image->width = width;
  image->height = height;
  image->palette_color_count = DOT_IMAGE_PALETTE_COLOR_COUNT_MAX;

  for (
    int color_index = 0;
    color_index < DOT_IMAGE_PALETTE_COLOR_COUNT_MAX;
    color_index++
  ) {

    image->palette_rgb565[color_index] = DEFAULT_PALETTE_RGB565[color_index];
  }

  memset(image->pixel_bytes, 0, (size_t)pixel_and_hitbox_byte_length);

  calculate_painted_and_hitbox_bounds(image);

  *created_image = image;

  return DOT_IMAGE_STATUS_OK;
}

void
free_dot_image(DotImage *image) {
  free(image);
}

DotImageStatus
save_dot_image_file(
  const DotImage *image,
  const char *path,
  int path_byte_length
) {

  char resolved_path[AREA512_PATH_MAX];

  if (
    !resolve_dot_image_path(
      path,
      path_byte_length,
      resolved_path,
      (int)sizeof resolved_path
    )
  )
    return DOT_IMAGE_STATUS_WRITE_FAILED;

  FILE *file = fopen(resolved_path, "wb");

  if (file == NULL)
    return DOT_IMAGE_STATUS_WRITE_FAILED;

  uint8_t header_bytes[DOT_IMAGE_HEADER_BYTE_LENGTH];

  memcpy(header_bytes, DOT_IMAGE_MAGIC, DOT_IMAGE_MAGIC_BYTE_LENGTH);

  header_bytes[4] = DOT_IMAGE_VERSION;
  header_bytes[5] = (uint8_t)image->width;
  header_bytes[6] = (uint8_t)image->height;
  header_bytes[7] = (uint8_t)image->palette_color_count;

  int saved =
    fwrite(header_bytes, 1, sizeof header_bytes, file) == sizeof header_bytes;

  for (
    int color_index = 0;
    saved && color_index < image->palette_color_count;
    color_index++
  ) {

    uint8_t palette_bytes[2];

    palette_bytes[0] = (uint8_t)(image->palette_rgb565[color_index] & 0xFF);
    palette_bytes[1] = (uint8_t)(image->palette_rgb565[color_index] >> 8);

    saved =
      fwrite(palette_bytes, 1, sizeof palette_bytes, file) ==
      sizeof palette_bytes;
  }

  if (saved) {
    int pixel_and_hitbox_byte_length =
      calculate_pixel_and_hitbox_byte_length(image->width, image->height);

    size_t written_byte_count =
      fwrite(
        image->pixel_bytes,
        1,
        (size_t)pixel_and_hitbox_byte_length,
        file
      );

    saved = written_byte_count == (size_t)pixel_and_hitbox_byte_length;
  }

  if (fclose(file) != 0)
    saved = 0;

  return saved ? DOT_IMAGE_STATUS_OK : DOT_IMAGE_STATUS_WRITE_FAILED;
}

int
fetch_dot_image_color_index(const DotImage *image, int dot_x, int dot_y) {
  if (
    dot_x < 0 ||
    dot_x >= image->width ||
    dot_y < 0 ||
    dot_y >= image->height
  )
    return DOT_IMAGE_TRANSPARENT_COLOR_INDEX;

  int pixel_byte_index = dot_y * (image->width / 2) + dot_x / 2;
  uint8_t pixel_byte = image->pixel_bytes[pixel_byte_index];

  if (dot_x % 2 == 0)
    return (pixel_byte >> 4) & 0x0F;

  return pixel_byte & 0x0F;
}

void
set_dot_image_color_index(
  DotImage *image,
  int dot_x,
  int dot_y,
  int color_index
) {

  if (
    dot_x < 0 || dot_x >= image->width || dot_y < 0 ||
    dot_y >= image->height
  )
    return;

  int pixel_byte_index = dot_y * (image->width / 2) + dot_x / 2;
  uint8_t pixel_byte = image->pixel_bytes[pixel_byte_index];

  if (dot_x % 2 == 0)
    pixel_byte = (uint8_t)((pixel_byte & 0x0F) | ((color_index & 0x0F) << 4));
  else
    pixel_byte = (uint8_t)((pixel_byte & 0xF0) | (color_index & 0x0F));

  image->pixel_bytes[pixel_byte_index] = pixel_byte;
}

int
has_dot_image_hitbox(const DotImage *image, int dot_x, int dot_y) {
  if (
    dot_x < 0 ||
    dot_x >= image->width ||
    dot_y < 0 ||
    dot_y >= image->height
  )
    return 0;

  const uint8_t *hitbox_bytes =
    image->pixel_bytes +
    calculate_pixel_byte_length(image->width, image->height);

  uint8_t hitbox_byte =
    hitbox_bytes[calculate_hitbox_byte_index(image, dot_x, dot_y)];

  return (hitbox_byte >> (7 - dot_x % 8)) & 1;
}

void
set_dot_image_hitbox(DotImage *image, int dot_x, int dot_y, int has_hitbox) {
  if (
    dot_x < 0 ||
    dot_x >= image->width ||
    dot_y < 0 ||
    dot_y >= image->height
  )
    return;

  uint8_t *hitbox_bytes =
    image->pixel_bytes +
    calculate_pixel_byte_length(image->width, image->height);

  int hitbox_byte_index = calculate_hitbox_byte_index(image, dot_x, dot_y);
  uint8_t dot_mask = (uint8_t)(1 << (7 - dot_x % 8));

  if (has_hitbox)
    hitbox_bytes[hitbox_byte_index] |= dot_mask;
  else
    hitbox_bytes[hitbox_byte_index] &= (uint8_t)~dot_mask;
}

void
push_dot_image_to_sprite(
  const DotImage *image,
  void *sprite,
  int screen_x,
  int screen_y
) {

  for (int dot_y = 0; dot_y < image->height; dot_y++) {
    for (int dot_x = 0; dot_x < image->width; dot_x++) {
      int color_index = fetch_dot_image_color_index(image, dot_x, dot_y);

      if (color_index == DOT_IMAGE_TRANSPARENT_COLOR_INDEX)
        continue;

      uint32_t rgb888 =
        convert_rgb565_to_rgb888(image->palette_rgb565[color_index]);

      area512_sprite_pixel(sprite, screen_x + dot_x, screen_y + dot_y, rgb888);
    }
  }
}

static int
has_painted_or_hitbox_dot(
  const DotImage *image,
  int dot_x,
  int dot_y,
  int use_hitbox
) {

  if (use_hitbox)
    return has_dot_image_hitbox(image, dot_x, dot_y);

  return fetch_dot_image_color_index(image, dot_x, dot_y) !=
         DOT_IMAGE_TRANSPARENT_COLOR_INDEX;
}

int
has_dot_image_overlap(
  const DotImage *first_image,
  int first_screen_x,
  int first_screen_y,
  const DotImage *second_image,
  int second_screen_x,
  int second_screen_y,
  int use_hitbox
) {

  int first_bounds_top =
    use_hitbox ? first_image->hitbox_top : first_image->painted_top;

  int first_bounds_bottom =
    use_hitbox ? first_image->hitbox_bottom : first_image->painted_bottom;

  int first_bounds_left =
    use_hitbox ? first_image->hitbox_left : first_image->painted_left;

  int first_bounds_right =
    use_hitbox ? first_image->hitbox_right : first_image->painted_right;

  for (
    int first_dot_y = first_bounds_top;
    first_dot_y <= first_bounds_bottom;
    first_dot_y++
  ) {

    for (
      int first_dot_x = first_bounds_left;
      first_dot_x <= first_bounds_right;
      first_dot_x++
    ) {

      if (
        !has_painted_or_hitbox_dot(
          first_image,
          first_dot_x,
          first_dot_y,
          use_hitbox
        )
      )
        continue;

      int second_dot_x = first_screen_x + first_dot_x - second_screen_x;
      int second_dot_y = first_screen_y + first_dot_y - second_screen_y;

      if (
        !has_painted_or_hitbox_dot(
          second_image,
          second_dot_x,
          second_dot_y,
          use_hitbox
        )
      )
        continue;

      return 1;
    }
  }

  return 0;
}

uint32_t
convert_rgb565_to_rgb888(uint16_t rgb565) {
  uint32_t red = (uint32_t)((rgb565 >> 11) & 0x1F);
  uint32_t green = (uint32_t)((rgb565 >> 5) & 0x3F);
  uint32_t blue = (uint32_t)(rgb565 & 0x1F);

  red = (red * 255 + 15) / 31;
  green = (green * 255 + 31) / 63;
  blue = (blue * 255 + 15) / 31;

  return (red << 16) | (green << 8) | blue;
}

#endif
