
#include <lgfx/v1/LGFXBase.hpp>
#include <lgfx/v1/LGFX_Sprite.hpp>

#include <ctype.h>
#include <new>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "area512_display.h"
#include "sdkconfig.h"
#include <littlefs.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// efontJA fonts so full-width glyphs render, matching the terminal.
#define AREA512_SPRITE_FONT (&lgfx::v1::fonts::efontJA_10)
#define AREA512_FILER_FONT (&lgfx::v1::fonts::efontJA_12)

static lgfx::v1::LGFX_Device *
area512_gfx_device(void) {
  return static_cast<lgfx::v1::LGFX_Device *>(area512_display_device());
}

static void *
area512_sprite_new_with_font(int w, int h, const lgfx::v1::IFont *font) {
  lgfx::v1::LGFX_Device *dev = area512_gfx_device();
  if (dev == nullptr || w <= 0 || h <= 0) {
    return nullptr;
  }

  lgfx::v1::LGFX_Sprite *spr = new (std::nothrow) lgfx::v1::LGFX_Sprite(dev);
  if (spr == nullptr) {
    return nullptr;
  }

  spr->setColorDepth(16);

  // Cardputer has no PSRAM; keep the sprite in internal RAM.
  spr->setPsram(false);
  if (spr->createSprite(w, h) == nullptr) {
    delete spr;
    return nullptr;
  }

  spr->setFont(font);
  spr->setTextSize(1);
  spr->setTextDatum(lgfx::v1::textdatum_t::top_left);
  spr->fillScreen((uint32_t)0x000000);

  return spr;
}

static const lgfx::v1::IFont *
area512_sprite_font_for_size(int font_size) {
  return font_size == 12 ? AREA512_FILER_FONT : AREA512_SPRITE_FONT;
}

static bool
read_lfs_line(lfs_file_t *file, char *line, int line_size) {
  int len = 0;

  while (len < line_size - 1) {
    char ch;
    lfs_ssize_t n = lfs_file_read(littlefs_get_lfs(), file, &ch, 1);

    if (n < 0)
      return false;

    if (n == 0)
      break;

    line[len++] = ch;

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
  if (!isxdigit((unsigned char)p[0]) || !isxdigit((unsigned char)p[1])) {
    *cursor = p;
    return false;
  }

  char hex[3] = {p[0], p[1], 0};
  *out = (uint8_t)strtoul(hex, nullptr, 16);
  *cursor = p + 2;

  return true;
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
  return area512_sprite_new_with_font(
    w,
    h,
    area512_sprite_font_for_size(font_size)
  );
}

void
area512_sprite_delete(void *p) {
  if (p == nullptr) {
    return;
  }

  lgfx::v1::LGFX_Sprite *spr = static_cast<lgfx::v1::LGFX_Sprite *>(p);
  spr->deleteSprite();

  delete spr;
}

int
area512_sprite_width(void *p) {
  if (p == nullptr)
    return 0;

  return (int)static_cast<lgfx::v1::LGFX_Sprite *>(p)->width();
}

int
area512_sprite_height(void *p) {
  if (p == nullptr)
    return 0;

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

  static_cast<lgfx::v1::LGFX_Sprite *>(p)->drawPixel(x, y, color);
}

void
area512_sprite_line(void *p, int x0, int y0, int x1, int y1, uint32_t color) {
  if (p == nullptr)
    return;

  static_cast<lgfx::v1::LGFX_Sprite *>(p)->drawLine(x0, y0, x1, y1, color);
}

void
area512_sprite_rect(void *p, int x, int y, int w, int h, uint32_t color) {
  if (p == nullptr)
    return;

  static_cast<lgfx::v1::LGFX_Sprite *>(p)->drawRect(x, y, w, h, color);
}

void
area512_sprite_fill_rect(void *p, int x, int y, int w, int h, uint32_t color) {
  if (p == nullptr)
    return;

  static_cast<lgfx::v1::LGFX_Sprite *>(p)->fillRect(x, y, w, h, color);
}

void
area512_sprite_circle(void *p, int x, int y, int r, uint32_t color) {
  if (p == nullptr)
    return;

  static_cast<lgfx::v1::LGFX_Sprite *>(p)->drawCircle(x, y, r, color);
}

void
area512_sprite_fill_circle(void *p, int x, int y, int r, uint32_t color) {
  if (p == nullptr)
    return;

  static_cast<lgfx::v1::LGFX_Sprite *>(p)->fillCircle(x, y, r, color);
}

// Transparent background; x,y is top-left.
void
area512_sprite_text(void *p, int x, int y, const char *str, uint32_t color) {
  if (p == nullptr || str == nullptr)
    return;

  lgfx::v1::LGFX_Sprite *spr = static_cast<lgfx::v1::LGFX_Sprite *>(p);
  spr->setTextColor(color);
  spr->drawString(str, x, y);
}

// -----------------------------------------------------------------------------
// Sprite transfer to screen
// -----------------------------------------------------------------------------

void
area512_sprite_push(void *p, int x, int y) {
  if (p == nullptr)
    return;

  lgfx::v1::LGFX_Sprite *spr = static_cast<lgfx::v1::LGFX_Sprite *>(p);
  spr->pushSprite(x, y);

  lgfx::v1::LGFX_Device *dev = area512_gfx_device();
  if (dev)
    dev->waitDMA();
}

// Overlay, keying out transp-colored pixels.
void
area512_sprite_push_transparent(void *p, int x, int y, uint32_t transp) {
  if (p == nullptr)
    return;

  lgfx::v1::LGFX_Sprite *spr = static_cast<lgfx::v1::LGFX_Sprite *>(p);
  spr->pushSprite(x, y, transp);

  lgfx::v1::LGFX_Device *dev = area512_gfx_device();
  if (dev)
    dev->waitDMA();
}

// -----------------------------------------------------------------------------
// Screen (shared device) operations
// -----------------------------------------------------------------------------

int
area512_gfx_width(void) {
  lgfx::v1::LGFX_Device *dev = area512_gfx_device();

  return dev ? (int)dev->width() : 0;
}

int
area512_gfx_height(void) {
  lgfx::v1::LGFX_Device *dev = area512_gfx_device();

  return dev ? (int)dev->height() : 0;
}

void
area512_gfx_fill_screen(uint32_t color) {
  lgfx::v1::LGFX_Device *dev = area512_gfx_device();

  if (dev)
    dev->fillScreen(color);
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

int
area512_gfx_show_header_image(const char *path, int hold_milliseconds) {
  lgfx::v1::LGFX_Device *dev = area512_gfx_device();
  if (dev == nullptr || path == nullptr)
    return 0;

  int err = littlefs_ensure_mounted();
  if (err < 0)
    return 0;

  lfs_file_t file;
  err = lfs_file_open(littlefs_get_lfs(), &file, path, LFS_O_RDONLY);
  if (err < 0)
    return 0;

  // Images are authored at the panel's exact size, so use its dimensions.
  const int img_w = dev->width();
  const int img_h = dev->height();
  bool in_array = false;
  bool ok = false;
  char line[192];

  while (read_lfs_line(&file, line, sizeof(line))) {
    if (strchr(line, '{')) {
      in_array = true;
      break;
    }
  }

  const int rowbytes = (img_w + 7) / 8;
  uint8_t *row = nullptr;

  if (in_array && rowbytes > 0) {
    row = (uint8_t *)malloc((size_t)rowbytes);
  }

  if (row != nullptr) {
    // LGFX converts RGB888 to the panel's color depth itself.
    const lgfx::v1::rgb888_t color_on(0xF5972D);
    const lgfx::v1::rgb888_t color_off(0x000000);
    int y = 0;
    int col = 0;

    while (read_lfs_line(&file, line, sizeof(line)) && y < img_h) {
      const char *p = line;
      uint8_t byte;

      while (parse_hex_byte(&p, &byte)) {
        row[col++] = byte;

        if (col >= rowbytes) {
          dev->startWrite();
          dev->drawBitmap(0, y, row, img_w, 1, color_on, color_off);
          dev->endWrite();
          ++y;
          col = 0;

          if (y >= img_h)
            break;
        }
      }
    }

    ok = (y >= img_h);

    free(row);

    if (ok && hold_milliseconds > 0) {
      vTaskDelay(pdMS_TO_TICKS(hold_milliseconds));
    }
  }

  lfs_file_close(littlefs_get_lfs(), &file);
  return ok ? 1 : 0;
}

} // extern "C"
