
#include <lgfx/v1/LGFXBase.hpp>
#include <lgfx/v1/LGFX_Sprite.hpp>

#include <ctype.h>
#include <new>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "area512_display.h"
#include "area512_crt.h"
#include "area512_quaternary_crt.h"
#include "area512_hal.h"
#include "sdkconfig.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define AREA512_SPRITE_FONT (&lgfx::v1::fonts::efontJA_10)
#define AREA512_FILER_FONT (&lgfx::v1::fonts::efontJA_12)

static constexpr int SPRITE_PIXEL_BYTE_SIZE = 2;

static constexpr bool CORE_SHIFTS_GREEN_AND_NARROWS_BLUE = false;

static constexpr int SOFT_KEYBOARD_WINDOW_GAP = 8;
static constexpr int WINDOW_FRAME_WIDTH = 1;

static constexpr int ROW_SLOT_WIDTH = 320;
static constexpr int ROW_SLOT_HEIGHT = 13;
static constexpr int ROW_SLOT_COUNT = 2;

static constexpr size_t SCREEN_SLOT_BYTE_SIZE =
  (size_t)ROW_SLOT_WIDTH *
  (AREA512_SCREEN_REGION_ROW_COUNT + AREA512_CRT_RADIUS * 4) *
  SPRITE_PIXEL_BYTE_SIZE;

static constexpr size_t ROW_SLOT_BYTE_SIZE = (size_t)ROW_SLOT_WIDTH *
  ROW_SLOT_HEIGHT * SPRITE_PIXEL_BYTE_SIZE;

static constexpr int SPRITE_SLOT_COUNT = ROW_SLOT_COUNT + 1;
static constexpr int SCREEN_SLOT_INDEX = ROW_SLOT_COUNT;

typedef struct {
  uint8_t *buffer;
  size_t buffer_byte_size;
  void *occupying_sprite;
} SpriteSlot;

alignas(4) static uint8_t s_screen_slot_buffer[SCREEN_SLOT_BYTE_SIZE];

alignas(4) static uint8_t
  s_row_slot_buffers[ROW_SLOT_COUNT][ROW_SLOT_BYTE_SIZE];

static SpriteSlot s_sprite_slots[SPRITE_SLOT_COUNT] = {
  {
    s_row_slot_buffers[0],
    ROW_SLOT_BYTE_SIZE,
    nullptr
  },
  {
    s_row_slot_buffers[1],
    ROW_SLOT_BYTE_SIZE,
    nullptr
  },
  {
    s_screen_slot_buffer,
    SCREEN_SLOT_BYTE_SIZE,
    nullptr
  },
};

static lgfx::v1::LGFX_Sprite *s_screen_sprite;
static int s_screen_buffer_first_row;
static int s_screen_buffer_row_count;
static int s_screen_destination_row_count;
static int s_screen_destination_first_row;
static int s_screen_next_row;
static int s_window_x;
static int s_window_y;
static int s_window_width;
static int s_window_height;
static int s_subwindow_x;
static int s_subwindow_y;
static int s_subwindow_width;
static int s_subwindow_height;
static pthread_mutex_t s_display_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool s_is_app_screen_shown = false;
static void (*s_draw_app_background)(void *context) = nullptr;
static void *s_app_background_context = nullptr;

static SpriteSlot *
find_free_sprite_slot(size_t required_byte_size) {
  for (int slot_index = 0; slot_index < SPRITE_SLOT_COUNT; slot_index++) {
    SpriteSlot *slot = &s_sprite_slots[slot_index];

    if (
      slot->occupying_sprite == nullptr &&
      slot->buffer_byte_size >= required_byte_size
    ) {

      return slot;
    }
  }

  return nullptr;
}

static SpriteSlot *
find_sprite_slot_holding(const void *sprite) {
  for (int slot_index = 0; slot_index < SPRITE_SLOT_COUNT; slot_index++) {
    SpriteSlot *slot = &s_sprite_slots[slot_index];

    if (slot->occupying_sprite == sprite)
      return slot;
  }

  return nullptr;
}

static lgfx::v1::LGFX_Device *
area512_gfx_device(void) {
  return static_cast<lgfx::v1::LGFX_Device *>(area512_display_device());
}

static int
find_drawable_bottom(lgfx::v1::LGFX_Device *dev) {
  if (area512_is_soft_keyboard_open())
    return AREA512_SOFT_KEYBOARD_TOP;

  return (int)dev->height();
}

static int
subtract_screen_buffer_origin(const void *p, int y) {
  if (p == s_screen_sprite)
    return y - s_screen_buffer_first_row;

  return y;
}

static void
set_sprite_text_style(
  lgfx::v1::LGFX_Sprite *sprite,
  const lgfx::v1::IFont *font
) {

  sprite->setFont(font);
  sprite->setTextSize(1);
  sprite->setTextDatum(lgfx::v1::textdatum_t::top_left);
}

static bool
assign_sprite_buffer(
  lgfx::v1::LGFX_Sprite *sprite,
  int width,
  int height,
  size_t required_byte_size
) {

  SpriteSlot *slot = find_free_sprite_slot(required_byte_size);

  if (slot != nullptr) {
    sprite->setBuffer(slot->buffer, width, height);
    slot->occupying_sprite = sprite;

    return true;
  }

  sprite->setPsram(false);

  return sprite->createSprite(width, height) != nullptr;
}

static void *
area512_sprite_new_with_font(int w, int h, const lgfx::v1::IFont *font) {
  lgfx::v1::LGFX_Device *dev = area512_gfx_device();
  if (dev == nullptr || w <= 0 || h <= 0) {
    return nullptr;
  }

  lgfx::v1::LGFX_Sprite *spr =
    new (std::nothrow) lgfx::v1::LGFX_Sprite(dev);

  if (spr == nullptr) {
    return nullptr;
  }

  spr->setColorDepth(16);

  size_t required_byte_size = (size_t)w * (size_t)h * SPRITE_PIXEL_BYTE_SIZE;

  if (!assign_sprite_buffer(spr, w, h, required_byte_size)) {
    delete spr;
    return nullptr;
  }

  set_sprite_text_style(spr, font);
  spr->fillScreen((uint32_t)0x000000);

  return spr;
}

static void *
create_quaternary_sprite_with_font(
  int width,
  int height,
  const lgfx::v1::IFont *font
) {

  lgfx::v1::LGFX_Device *device = area512_gfx_device();

  if (device == nullptr || width <= 0 || height <= 0)
    return nullptr;

  lgfx::v1::LGFX_Sprite *sprite =
    new (std::nothrow) lgfx::v1::LGFX_Sprite(device);

  if (sprite == nullptr)
    return nullptr;

  sprite->setColorDepth(lgfx::v1::palette_4bit);

  size_t required_byte_size = ((size_t)width + 1) / 2 * (size_t)height;

  if (!assign_sprite_buffer(sprite, width, height, required_byte_size)) {
    delete sprite;
    return nullptr;
  }

  if (!sprite->createPalette()) {
    area512_sprite_delete(sprite);
    return nullptr;
  }

  sprite->setPaletteColor(
    AREA512_QUATERNARY_SPRITE_BACKGROUND_COLOR_INDEX,
    area512_theme_background_color()
  );

  sprite->setPaletteColor(
    AREA512_QUATERNARY_SPRITE_TEXT_COLOR_INDEX,
    area512_theme_text_color()
  );

  sprite->setPaletteColor(
    AREA512_QUATERNARY_SPRITE_EMPHASIS_COLOR_INDEX,
    area512_theme_emphasis_color()
  );

  sprite->setPaletteColor(
    AREA512_QUATERNARY_SPRITE_BORDER_COLOR_INDEX,
    area512_theme_border_color()
  );

  sprite->setPaletteColor(
    AREA512_QUATERNARY_SPRITE_SELECTED_COLOR_INDEX,
    area512_theme_selected_color()
  );

  sprite->setPaletteColor(
    AREA512_QUATERNARY_SPRITE_BOX_COLOR_INDEX,
    area512_theme_box_color()
  );

  set_sprite_text_style(sprite, font);

  sprite->fillScreen(
    (uint32_t)AREA512_QUATERNARY_SPRITE_BACKGROUND_COLOR_INDEX
  );

  return sprite;
}

static const lgfx::v1::IFont *
area512_sprite_font_for_size(int font_size) {
  switch (font_size) {
  case 6:
    return &lgfx::v1::fonts::TomThumb;
  case 8:
    return &lgfx::v1::fonts::Font0;
  case 10:
    return AREA512_SPRITE_FONT;
  case 12:
    return AREA512_FILER_FONT;
  case 14:
    return &lgfx::v1::fonts::efontJA_14;
  case 16:
    return &lgfx::v1::fonts::efontJA_16;
  case 24:
    return &lgfx::v1::fonts::efontJA_24;
  default:
    return nullptr;
  }
}

static int
area512_sprite_font_height_for_size(int font_size) {
  switch (font_size) {
  case 6:
    return 6;
  case 8:
    return 8;
  case 12:
    return 13;
  case 14:
    return 15;
  case 16:
    return 17;
  case 24:
    return 25;
  case 10:
    return 12;
  default:
    return 0;
  }
}

static bool
read_text_line(FILE *file, char *line, int line_size) {
  int len = 0;

  while (len < line_size - 1) {
    int ch = fgetc(file);

    if (ch == EOF)
      break;

    line[len++] = (char)ch;

    if (ch == '\n')
      break;
  }

  line[len] = 0;

  return len > 0;
}

static bool
parse_hex_byte(const char **cursor, uint8_t *out) {
  const char *p = *cursor;

  while (*p && !(p[0] == '0' && (p[1] == 'x' || p[1] == 'X'))) {
    ++p;
  }

  if (!*p) {
    *cursor = p;

    return false;
  }

  p += 2;

  if (
    !isxdigit((unsigned char)p[0]) || !isxdigit((unsigned char)p[1])
  ) {
    *cursor = p;
    return false;
  }

  char hex[3] = {p[0], p[1], 0};
  *out = (uint8_t)strtoul(hex, nullptr, 16);
  *cursor = p + 2;

  return true;
}

static constexpr int BITMAP_WIDTH = 240;
static constexpr int BITMAP_HEIGHT = 135;
static constexpr int BITMAP_ROW_BYTE_SIZE = (BITMAP_WIDTH + 7) / 8;

static constexpr size_t BITMAP_BYTE_SIZE =
  (size_t)BITMAP_ROW_BYTE_SIZE * BITMAP_HEIGHT;

static void
push_bitmap_scaled_to_screen(
  const uint8_t *bitmap,
  int line_delay_milliseconds
) {

  lgfx::v1::LGFX_Device *dev = area512_gfx_device();

  if (dev == nullptr)
    return;

  int screen_width = area512_gfx_width();
  int screen_height = area512_gfx_height();
  int draw_height = BITMAP_HEIGHT * screen_width / BITMAP_WIDTH;

  if (draw_height > screen_height)
    return;

  int first_row = (screen_height - draw_height) / 2;

  if (first_row > 0)
    area512_gfx_fill_screen(area512_theme_background_color());

  uint32_t set_bit_color;
  uint32_t clear_bit_color;

  area512_theme_pick_bitmap_colors(
    &set_bit_color,
    &clear_bit_color
  );

  void *line_handle = area512_sprite_new(screen_width, 1);

  if (line_handle == nullptr)
    return;

  lgfx::v1::LGFX_Sprite *line_sprite =
    static_cast<lgfx::v1::LGFX_Sprite *>(line_handle);

  TickType_t delay = pdMS_TO_TICKS(line_delay_milliseconds);

  if (line_delay_milliseconds > 0 && delay == 0)
    delay = 1;

  for (int row = 0; row < draw_height; row++) {
    const uint8_t *source_row =
      bitmap + (size_t)(row * BITMAP_HEIGHT /
      draw_height) * BITMAP_ROW_BYTE_SIZE;

    for (int column = 0; column < screen_width; column++) {
      int source_column = column * BITMAP_WIDTH / screen_width;
      uint8_t source_byte = source_row[source_column >> 3];
      bool bit_is_set = (source_byte & (0x80 >> (source_column & 7))) != 0;

      line_sprite->drawPixel(
        column,
        0,
        bit_is_set ? set_bit_color : clear_bit_color
      );
    }

    area512_sprite_push(line_handle, 0, first_row + row);

    if (delay > 0)
      vTaskDelay(delay);
  }

  area512_sprite_delete(line_handle);
}

extern "C" {

// -----------------------------------------------------------------------------
// Sprite lifecycle
// -----------------------------------------------------------------------------

void *
area512_sprite_new(int w, int h) {
  return area512_sprite_new_with_font(w, h, AREA512_SPRITE_FONT);
}

void *
area512_sprite_new_with_font_size(int w, int h, int font_size) {
  const lgfx::v1::IFont *font = area512_sprite_font_for_size(font_size);

  if (font == nullptr)
    return nullptr;

  return area512_sprite_new_with_font(w, h, font);
}

void *
area512_quaternary_sprite_new(int width, int height) {
  return create_quaternary_sprite_with_font(width, height, AREA512_SPRITE_FONT);
}

void *
area512_quaternary_sprite_new_with_font_size(
  int width,
  int height,
  int font_size
) {

  const lgfx::v1::IFont *font = area512_sprite_font_for_size(font_size);

  if (font == nullptr)
    return nullptr;

  return create_quaternary_sprite_with_font(width, height, font);
}

void
area512_sprite_set_font_size(void *p, int font_size) {
  if (p == nullptr)
    return;

  const lgfx::v1::IFont *font = area512_sprite_font_for_size(font_size);

  if (font == nullptr)
    return;

  set_sprite_text_style(static_cast<lgfx::v1::LGFX_Sprite *>(p), font);
}

int
area512_sprite_font_height(int font_size) {
  return area512_sprite_font_height_for_size(font_size);
}

void
area512_sprite_set_title_font(void *p) {
  if (p == nullptr)
    return;

  lgfx::v1::LGFX_Sprite *spr =
    static_cast<lgfx::v1::LGFX_Sprite *>(p);

  spr->setFont(&lgfx::v1::fonts::Orbitron_Light_24);
  spr->setTextSize(0.75f);
  spr->setTextDatum(lgfx::v1::textdatum_t::top_left);
}

void
area512_sprite_delete(void *p) {
  if (p == nullptr) {
    return;
  }

  lgfx::v1::LGFX_Sprite *spr =
    static_cast<lgfx::v1::LGFX_Sprite *>(p);

  SpriteSlot *slot = find_sprite_slot_holding(spr);

  if (slot != nullptr)
    slot->occupying_sprite = nullptr;

  if (spr == s_screen_sprite)
    s_screen_sprite = nullptr;

  spr->deleteSprite();

  delete spr;
}

int
area512_sprite_width(void *p) {
  if (p == nullptr)
    return 0;

  if (p == s_screen_sprite)
    return area512_gfx_width();

  return (int)static_cast<lgfx::v1::LGFX_Sprite *>(p)->width();
}

int
area512_sprite_height(void *p) {
  if (p == nullptr)
    return 0;

  if (p == s_screen_sprite)
    return area512_gfx_height();

  return (int)static_cast<lgfx::v1::LGFX_Sprite *>(p)->height();
}

// -----------------------------------------------------------------------------
// Sprite drawing
// -----------------------------------------------------------------------------

void
area512_sprite_fill(void *p, uint32_t color) {
  if (p == nullptr)
    return;

  static_cast<lgfx::v1::LGFX_Sprite *>(p)->fillScreen(color);
}

void
area512_sprite_pixel(void *p, int x, int y, uint32_t color) {
  if (p == nullptr)
    return;

  static_cast<lgfx::v1::LGFX_Sprite *>(p)->drawPixel(
    x,
    subtract_screen_buffer_origin(p, y),
    color
  );
}

void
area512_sprite_line(void *p, int x0, int y0, int x1, int y1, uint32_t color) {
  if (p == nullptr)
    return;

  static_cast<lgfx::v1::LGFX_Sprite *>(p)->drawLine(
    x0,
    subtract_screen_buffer_origin(p, y0),
    x1,
    subtract_screen_buffer_origin(p, y1),
    color
  );
}

void
area512_sprite_rect(void *p, int x, int y, int w, int h, uint32_t color) {
  if (p == nullptr)
    return;

  static_cast<lgfx::v1::LGFX_Sprite *>(p)->drawRect(
    x,
    subtract_screen_buffer_origin(p, y),
    w,
    h,
    color
  );
}

void
area512_sprite_fill_rect(void *p, int x, int y, int w, int h, uint32_t color) {
  if (p == nullptr)
    return;

  static_cast<lgfx::v1::LGFX_Sprite *>(p)->fillRect(
    x,
    subtract_screen_buffer_origin(p, y),
    w,
    h,
    color
  );
}

void
area512_sprite_blend_rect(
  void *p,
  int x,
  int y,
  int w,
  int h,
  uint32_t color,
  int opacity
) {

  if (p == nullptr || w <= 0 || h <= 0 || opacity <= 0)
    return;

  if (opacity >= 100) {
    area512_sprite_fill_rect(p, x, y, w, h, color);
    return;
  }

  lgfx::v1::LGFX_Sprite *spr = static_cast<lgfx::v1::LGFX_Sprite *>(p);
  y = subtract_screen_buffer_origin(p, y);
  int right = x + w;
  int bottom = y + h;

  if (x < 0)
    x = 0;
  if (y < 0)
    y = 0;
  if (right > spr->width())
    right = spr->width();
  if (bottom > spr->height())
    bottom = spr->height();

  for (int row = y; row < bottom; row++) {
    for (int column = x; column < right; column++) {
      uint32_t background = spr->readPixelRGB(column, row).RGB888();
      uint32_t blended = 0;

      for (int shift = 0; shift <= 16; shift += 8) {
        uint32_t channel =
          (((background >> shift) & 0xFF) * (100 - opacity) +
          ((color >> shift) & 0xFF) * opacity) / 100;

        blended |= channel << shift;
      }

      spr->drawPixel(column, row, blended);
    }
  }
}

void
area512_sprite_round_rect(
  void *p,
  int x,
  int y,
  int w,
  int h,
  int corner_radius,
  uint32_t color
) {

  if (p == nullptr)
    return;

  static_cast<lgfx::v1::LGFX_Sprite *>(p)->drawRoundRect(
    x,
    subtract_screen_buffer_origin(p, y),
    w,
    h,
    corner_radius,
    color
  );
}

void
area512_sprite_circle(void *p, int x, int y, int r, uint32_t color) {
  if (p == nullptr)
    return;

  static_cast<lgfx::v1::LGFX_Sprite *>(p)->drawCircle(
    x,
    subtract_screen_buffer_origin(p, y),
    r,
    color
  );
}

void
area512_sprite_fill_circle(void *p, int x, int y, int r, uint32_t color) {
  if (p == nullptr)
    return;

  static_cast<lgfx::v1::LGFX_Sprite *>(p)->fillCircle(
    x,
    subtract_screen_buffer_origin(p, y),
    r,
    color
  );
}

void
area512_sprite_text(void *p, int x, int y, const char *str, uint32_t color) {
  if (p == nullptr || str == nullptr)
    return;

  lgfx::v1::LGFX_Sprite *spr = static_cast<lgfx::v1::LGFX_Sprite *>(p);
  spr->setTextColor(color);
  spr->drawString(str, x, subtract_screen_buffer_origin(p, y));
}

int
area512_sprite_text_width(void *p, const char *str) {
  if (p == nullptr || str == nullptr)
    return 0;

  return (int)static_cast<lgfx::v1::LGFX_Sprite *>(p)->textWidth(str);
}

// -----------------------------------------------------------------------------
// Sprite transfer to screen
// -----------------------------------------------------------------------------

static void
push_sprite_clipped(
  void *p,
  int x,
  int y,
  int visible_x,
  int visible_y,
  int visible_width,
  int visible_height,
  bool has_transparent_color,
  uint32_t transparent_color
) {

  lgfx::v1::LGFX_Sprite *spr = static_cast<lgfx::v1::LGFX_Sprite *>(p);
  lgfx::v1::LGFX_Device *dev = area512_gfx_device();

  int32_t clip_x;
  int32_t clip_y;
  int32_t clip_width;
  int32_t clip_height;

  pthread_mutex_lock(&s_display_mutex);

  dev->getClipRect(&clip_x, &clip_y, &clip_width, &clip_height);
  dev->setClipRect(visible_x, visible_y, visible_width, visible_height);

  if (
    area512_theme_is_crt_enabled() &&
    spr->getColorDepth() == lgfx::v1::palette_4bit
  )
    area512_quaternary_crt_push(
      dev,
      spr,
      x,
      y,
      CORE_SHIFTS_GREEN_AND_NARROWS_BLUE,
      has_transparent_color,
      transparent_color
    );

  else if (area512_theme_is_crt_enabled())
    area512_crt_push(
      dev,
      spr,
      x,
      y,
      CORE_SHIFTS_GREEN_AND_NARROWS_BLUE,
      has_transparent_color,
      transparent_color,
      p == s_screen_sprite ? s_screen_destination_first_row - s_screen_buffer_first_row : 0,
      p == s_screen_sprite ? s_screen_destination_row_count : -1
    );

  else if (has_transparent_color)
    spr->pushSprite(dev, x, y, transparent_color);

  else
    spr->pushSprite(dev, x, y);

  dev->setClipRect(clip_x, clip_y, clip_width, clip_height);
  dev->waitDMA();

  pthread_mutex_unlock(&s_display_mutex);
}

static void
push_sprite(
  void *p,
  int x,
  int y,
  bool has_transparent_color,
  uint32_t transparent_color
) {

  lgfx::v1::LGFX_Device *dev = area512_gfx_device();

  if (p == nullptr || dev == nullptr)
    return;

  int window_top = area512_gfx_window_top();

  if (s_window_width > 0)
    push_sprite_clipped(
      p,
      x + s_window_x,
      y + window_top,
      s_window_x,
      window_top,
      s_window_width,
      s_window_height,
      has_transparent_color,
      transparent_color
    );
  else
    push_sprite_clipped(
      p,
      x,
      y,
      0,
      0,
      (int)dev->width(),
      find_drawable_bottom(dev),
      has_transparent_color,
      transparent_color
    );
}

void
area512_sprite_push(void *p, int x, int y) {
  push_sprite(p, x, y, false, 0);
}

void
area512_sprite_push_transparent(void *p, int x, int y, uint32_t transp) {
  push_sprite(p, x, y, true, transp);
}

// -----------------------------------------------------------------------------
// Whole-screen drawing, one band of rows at a time
// -----------------------------------------------------------------------------

void *
area512_screen_new(int font_size) {
  lgfx::v1::LGFX_Device *dev = area512_gfx_device();
  const lgfx::v1::IFont *font = area512_sprite_font_for_size(font_size);
  SpriteSlot *slot = &s_sprite_slots[SCREEN_SLOT_INDEX];

  if (dev == nullptr || font == nullptr || slot->occupying_sprite != nullptr)
    return nullptr;

  lgfx::v1::LGFX_Sprite *spr =
    new (std::nothrow) lgfx::v1::LGFX_Sprite(dev);

  if (spr == nullptr)
    return nullptr;

  spr->setColorDepth(16);

  spr->setBuffer(
    slot->buffer,
    area512_gfx_width(),
    AREA512_SCREEN_REGION_ROW_COUNT,
    16
  );

  set_sprite_text_style(spr, font);

  slot->occupying_sprite = spr;
  s_screen_sprite = spr;
  s_screen_buffer_first_row = 0;
  s_screen_buffer_row_count = 0;
  s_screen_destination_row_count = 0;
  s_screen_destination_first_row = 0;
  s_screen_next_row = 0;

  return spr;
}

static bool
assign_screen_buffer_and_destination_rows(
  int first_row,
  int row_count,
  bool changed
) {

  int screen_height = area512_gfx_height();

  if (area512_theme_is_crt_enabled()) {
    Area512CrtRegion region;

    if (!area512_crt_region(first_row, row_count, screen_height, changed, &region))
      return false;

    s_screen_buffer_first_row = region.source_first;
    s_screen_buffer_row_count = region.source_count;
    s_screen_destination_first_row = region.destination_first;
    s_screen_destination_row_count = region.destination_count;

    return true;
  }

  if (first_row < 0 || first_row >= screen_height || row_count <= 0)
    return false;

  if (row_count > screen_height - first_row)
    row_count = screen_height - first_row;

  s_screen_buffer_first_row = first_row;
  s_screen_buffer_row_count = row_count;
  s_screen_destination_first_row = first_row;
  s_screen_destination_row_count = row_count;

  return true;
}

int
area512_screen_begin_region(void *p) {
  lgfx::v1::LGFX_Device *dev = area512_gfx_device();

  if (p == nullptr || p != s_screen_sprite || dev == nullptr)
    return 0;

  int screen_width = area512_gfx_width();
  int screen_height = area512_gfx_height();

  if (s_screen_next_row >= screen_height) {
    s_screen_next_row = 0;
    s_screen_buffer_row_count = 0;
    s_screen_destination_row_count = 0;

    return 0;
  }

  int buffer_row_capacity =
    (int)(SCREEN_SLOT_BYTE_SIZE /
    ((size_t)screen_width * SPRITE_PIXEL_BYTE_SIZE));

  if (
    !assign_screen_buffer_and_destination_rows(
      s_screen_next_row,
      AREA512_SCREEN_REGION_ROW_COUNT,
      false
    )
  )
    return 0;

  if (s_screen_buffer_row_count > buffer_row_capacity) {
    s_screen_next_row = 0;
    s_screen_buffer_row_count = 0;
    s_screen_destination_row_count = 0;

    return 0;
  }

  s_screen_sprite->setBuffer(
    s_sprite_slots[SCREEN_SLOT_INDEX].buffer,
    screen_width,
    s_screen_buffer_row_count,
    16
  );

  s_screen_next_row += s_screen_destination_row_count;

  return 1;
}

int
area512_screen_begin_region_at(void *screen, int first_row, int row_count) {
  if (
    screen == nullptr ||
    screen != s_screen_sprite ||
    area512_gfx_device() == nullptr ||
    row_count > AREA512_SCREEN_REGION_ROW_COUNT ||
    !assign_screen_buffer_and_destination_rows(first_row, row_count, true)
  )
    return 0;

  s_screen_sprite->setBuffer(
    s_sprite_slots[SCREEN_SLOT_INDEX].buffer,
    area512_gfx_width(),
    s_screen_buffer_row_count,
    16
  );

  return 1;
}

void
area512_screen_draw_sprite(void *screen, void *sprite, int x, int y) {
  if (screen == nullptr || screen != s_screen_sprite || sprite == nullptr)
    return;

  static_cast<lgfx::v1::LGFX_Sprite *>(sprite)->pushSprite(
    s_screen_sprite,
    x,
    y - s_screen_buffer_first_row
  );
}

void
area512_screen_draw_sprite_clipped(
  void *screen,
  void *sprite,
  int x,
  int y,
  int clip_x,
  int clip_y,
  int clip_width,
  int clip_height
) {

  if (screen == nullptr || screen != s_screen_sprite || sprite == nullptr)
    return;

  int32_t saved_x;
  int32_t saved_y;
  int32_t saved_width;
  int32_t saved_height;

  s_screen_sprite->getClipRect(&saved_x, &saved_y, &saved_width, &saved_height);

  s_screen_sprite->setClipRect(
    clip_x,
    clip_y - s_screen_buffer_first_row,
    clip_width,
    clip_height
  );

  static_cast<lgfx::v1::LGFX_Sprite *>(sprite)->pushSprite(
    s_screen_sprite,
    x,
    y - s_screen_buffer_first_row
  );

  s_screen_sprite->setClipRect(saved_x, saved_y, saved_width, saved_height);
}

void
area512_screen_read_sprite(void *screen, void *sprite, int x, int y) {
  if (screen == nullptr || screen != s_screen_sprite || sprite == nullptr)
    return;

  s_screen_sprite->pushSprite(
    static_cast<lgfx::v1::LGFX_Sprite *>(sprite),
    -x,
    s_screen_buffer_first_row - y
  );
}

int
area512_screen_draw_rgb565(
  void *screen,
  const char *path,
  char *error,
  size_t error_size
) {

  if (
    screen == nullptr || screen != s_screen_sprite || path == nullptr || !*path
  )
    return 0;

  size_t image_bytes = 0;
  const uint8_t *image = area512_seed_find_file(path, &image_bytes);
  size_t row_bytes = (size_t)area512_gfx_width() * SPRITE_PIXEL_BYTE_SIZE;
  size_t screen_bytes = row_bytes * area512_gfx_height();
  bool drawn = false;

  if (image == nullptr) {
    snprintf(
      error,
      error_size,
      "Background: not in firmware"
    );

  } else if (image_bytes != screen_bytes) {
    snprintf(
      error,
      error_size,
      "Background: expected %u RGB565 bytes",
      (unsigned)screen_bytes
    );
  } else {
    // The 16-bit sprite buffer stores RGB565 with the high byte first.
    memcpy(
      s_screen_sprite->getBuffer(),
      image + row_bytes * s_screen_buffer_first_row,
      row_bytes * s_screen_buffer_row_count
    );

    drawn = true;
  }

  if (!drawn)
    s_screen_sprite->fillScreen(area512_theme_background_color());

  return drawn;
}

int
area512_screen_region_top(void *p) {
  if (p == nullptr || p != s_screen_sprite)
    return 0;

  return s_screen_buffer_first_row;
}

int
area512_screen_region_bottom(void *p) {
  if (p == nullptr || p != s_screen_sprite)
    return area512_gfx_height();

  return s_screen_buffer_first_row + s_screen_buffer_row_count;
}

void
area512_screen_push_region(void *p) {
  lgfx::v1::LGFX_Device *dev = area512_gfx_device();

  if (p == nullptr || p != s_screen_sprite || dev == nullptr)
    return;

  if (s_screen_destination_row_count <= 0)
    return;

  area512_sprite_push(
    s_screen_sprite,
    0,
    s_screen_buffer_first_row
  );

  dev->waitDMA();
}

int
area512_screen_draw(void *p) {
  area512_screen_push_region(p);
  return area512_screen_begin_region(p);
}

// -----------------------------------------------------------------------------
// Screen (shared device) operations
// -----------------------------------------------------------------------------

void
area512_gfx_set_window(int x, int y, int width, int height) {
  s_window_x = width > 0 ? x : 0;
  s_window_y = height > 0 ? y : 0;
  s_window_width = width;
  s_window_height = height;
}

int
area512_gfx_window_left(void) {
  return s_window_x;
}

int
area512_gfx_read_window_top_without_soft_keyboard(void) {
  return s_window_y;
}

void
area512_gfx_move_window(int screen_left, int screen_top_without_soft_keyboard) {
  if (s_window_width <= 0)
    return;

  s_window_x = screen_left;
  s_window_y = screen_top_without_soft_keyboard;
}

int
area512_gfx_find_window_top(int window_top, int window_height) {
  if (
    area512_is_soft_keyboard_open() &&
    window_top + window_height > AREA512_SOFT_KEYBOARD_TOP
  ) {

    int raised_window_top =
      AREA512_SOFT_KEYBOARD_TOP - SOFT_KEYBOARD_WINDOW_GAP - window_height;

    return raised_window_top > 0 ? raised_window_top : 0;
  }

  return window_top;
}

int
area512_gfx_window_top(void) {
  if (s_window_height <= 0)
    return 0;

  return area512_gfx_find_window_top(s_window_y, s_window_height);
}

void
area512_gfx_set_subwindow(int x, int y, int width, int height) {
  s_subwindow_x = x;
  s_subwindow_y = y;
  s_subwindow_width = width;
  s_subwindow_height = height;
}

void
area512_gfx_push_subwindow(void *sprite) {
  lgfx::v1::LGFX_Device *dev = area512_gfx_device();

  if (
    sprite == nullptr ||
    dev == nullptr ||
    s_subwindow_width <= 0 ||
    s_subwindow_height <= 0
  )
    return;

  int subwindow_x = s_window_x + s_subwindow_x;
  int subwindow_y = area512_gfx_window_top() + s_subwindow_y;
  int visible_height = s_subwindow_height;
  int drawable_bottom = find_drawable_bottom(dev);

  if (subwindow_y + visible_height > drawable_bottom)
    visible_height = drawable_bottom - subwindow_y;

  if (visible_height <= 0)
    return;

  push_sprite_clipped(
    sprite,
    subwindow_x,
    subwindow_y,
    subwindow_x,
    subwindow_y,
    s_subwindow_width,
    visible_height,
    false,
    0
  );
}

void
area512_gfx_set_app_background_drawer(
  void (*draw_app_background)(void *context),
  void *context
) {

  s_draw_app_background = draw_app_background;
  s_app_background_context = context;
}

void
area512_gfx_show_app_screen(void) {
  s_is_app_screen_shown = true;
}

void
area512_gfx_hide_app_screen(void) {
  s_is_app_screen_shown = false;
}

typedef struct {
  void *occupying_sprite;
  lgfx::v1::LGFX_Sprite *screen_sprite;
  int buffer_first_row;
  int buffer_row_count;
  int destination_row_count;
  int destination_first_row;
  int next_row;
} ScreenSlotState;

static ScreenSlotState

save_and_release_screen_slot(void) {
  SpriteSlot *slot = &s_sprite_slots[SCREEN_SLOT_INDEX];

  ScreenSlotState screen_slot_state = {
    slot->occupying_sprite,
    s_screen_sprite,
    s_screen_buffer_first_row,
    s_screen_buffer_row_count,
    s_screen_destination_row_count,
    s_screen_destination_first_row,
    s_screen_next_row
  };

  slot->occupying_sprite = nullptr;
  s_screen_sprite = nullptr;

  return screen_slot_state;
}

static void
restore_screen_slot(const ScreenSlotState *screen_slot_state) {
  s_sprite_slots[SCREEN_SLOT_INDEX].occupying_sprite =
    screen_slot_state->occupying_sprite;

  s_screen_sprite = screen_slot_state->screen_sprite;
  s_screen_buffer_first_row = screen_slot_state->buffer_first_row;
  s_screen_buffer_row_count = screen_slot_state->buffer_row_count;
  s_screen_destination_row_count = screen_slot_state->destination_row_count;
  s_screen_destination_first_row = screen_slot_state->destination_first_row;
  s_screen_next_row = screen_slot_state->next_row;
}

void
area512_gfx_draw_app_screen(void) {
  if (!s_is_app_screen_shown)
    return;

  if (s_draw_app_background != nullptr) {
    ScreenSlotState screen_slot_state = save_and_release_screen_slot();

    s_draw_app_background(s_app_background_context);

    restore_screen_slot(&screen_slot_state);
  }
}

int
area512_gfx_width(void) {
  lgfx::v1::LGFX_Device *dev = area512_gfx_device();

  if (dev == nullptr)
    return 0;

  if (s_window_width > 0)
    return s_window_width;

  return (int)dev->width();
}

int
area512_gfx_height(void) {
  lgfx::v1::LGFX_Device *dev = area512_gfx_device();

  if (dev == nullptr)
    return 0;

  if (s_window_height > 0)
    return s_window_height;

  return (int)dev->height();
}

int
area512_gfx_screen_width(void) {
  lgfx::v1::LGFX_Device *dev = area512_gfx_device();

  return dev ? (int)dev->width() : 0;
}

int
area512_gfx_screen_height(void) {
  lgfx::v1::LGFX_Device *dev = area512_gfx_device();

  return dev ? (int)dev->height() : 0;
}

void
area512_gfx_full_screen(void) {
  area512_gfx_set_window(0, 0, 0, 0);
}

void
area512_gfx_fill_screen(uint32_t color) {
  lgfx::v1::LGFX_Device *dev = area512_gfx_device();

  if (dev == nullptr)
    return;

  if (s_window_width > 0) {
    void *row = area512_sprite_new(s_window_width, 1);

    if (row == nullptr)
      return;

    area512_sprite_fill(row, color);

    for (int y = 0; y < s_window_height; y++)
      area512_sprite_push(row, 0, y);

    area512_sprite_delete(row);

    return;
  }

  pthread_mutex_lock(&s_display_mutex);

  dev->fillRect(
    0,
    0,
    (int)dev->width(),
    find_drawable_bottom(dev),
    color
  );

  pthread_mutex_unlock(&s_display_mutex);
}

void
area512_gfx_clear_soft_keyboard_area(void) {
  lgfx::v1::LGFX_Device *dev = area512_gfx_device();

  if (dev == nullptr)
    return;

  int cleared_top =
    AREA512_SOFT_KEYBOARD_TOP - SOFT_KEYBOARD_WINDOW_GAP + WINDOW_FRAME_WIDTH;

  pthread_mutex_lock(&s_display_mutex);

  dev->fillRect(
    0,
    cleared_top,
    (int)dev->width(),
    (int)dev->height() - cleared_top,
    area512_theme_background_color()
  );

  pthread_mutex_unlock(&s_display_mutex);
}

void
area512_gfx_draw_soft_keyboard_key(
  int x,
  int y,
  int width,
  int height,
  const char *label,
  bool is_pressed
) {

  lgfx::v1::LGFX_Device *dev = area512_gfx_device();

  if (dev == nullptr)
    return;

  pthread_mutex_lock(&s_display_mutex);

  dev->startWrite();

  dev->fillRect(
    x,
    y,
    width,
    height,
    is_pressed ? area512_theme_selected_color() : area512_theme_background_color()
  );

  dev->drawRect(x, y, width, height, area512_theme_border_color());
  dev->setFont(AREA512_SPRITE_FONT);
  dev->setTextSize(1);
  dev->setTextDatum(lgfx::v1::textdatum_t::middle_center);
  dev->setTextColor(area512_theme_text_color());
  dev->drawString(label, x + width / 2, y + height / 2);
  dev->endWrite();

  pthread_mutex_unlock(&s_display_mutex);
}

void
area512_gfx_set_brightness(int brightness) {
  lgfx::v1::LGFX_Device *dev = area512_gfx_device();

  if (dev == nullptr)
    return;

  if (brightness < 0)
    brightness = 0;

  if (brightness > 255)
    brightness = 255;

  dev->setBrightness((uint8_t)brightness);
}

void
area512_gfx_show_bitmap(const void *bitmap, int line_delay_milliseconds) {
  if (bitmap == nullptr)
    return;

  push_bitmap_scaled_to_screen(
    (const uint8_t *)bitmap,
    line_delay_milliseconds
  );
}

int
area512_gfx_load_header_image(
  const char *path,
  uint8_t *bitmap,
  size_t bitmap_size
) {

  if (path == nullptr || bitmap == nullptr)
    return 0;

  char resolved_path[AREA512_PATH_MAX];

  if (
    area512_resolve_data_path(
      path,
      resolved_path,
      sizeof resolved_path
    ) != 0
  )
    return 0;

  FILE *file = fopen(resolved_path, "rb");

  if (file == nullptr)
    return 0;

  char line[192];
  bool array_started = false;

  while (read_text_line(file, line, sizeof(line))) {
    if (strchr(line, '{')) {
      array_started = true;
      break;
    }
  }

  size_t filled_byte_count = 0;

  if (array_started) {
    while (
      filled_byte_count < bitmap_size &&
      read_text_line(file, line, sizeof(line))
    ) {

      const char *cursor = line;
      uint8_t parsed_byte;

      while (
        filled_byte_count < bitmap_size &&
        parse_hex_byte(&cursor, &parsed_byte)
      )
        bitmap[filled_byte_count++] = parsed_byte;
    }
  }

  fclose(file);

  return filled_byte_count == bitmap_size ? 1 : 0;
}

int
area512_gfx_show_header_image(const char *path, int hold_milliseconds) {
  uint8_t *bitmap = (uint8_t *)malloc(BITMAP_BYTE_SIZE);

  if (bitmap == nullptr)
    return 0;

  int loaded =
    area512_gfx_load_header_image(path, bitmap, BITMAP_BYTE_SIZE);

  if (loaded) {
    push_bitmap_scaled_to_screen(bitmap, 0);

    if (hold_milliseconds > 0)
      vTaskDelay(pdMS_TO_TICKS(hold_milliseconds));
  }

  free(bitmap);

  return loaded;
}

} // extern "C"
