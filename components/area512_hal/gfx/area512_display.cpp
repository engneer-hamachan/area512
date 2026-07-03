#include "area512_display.h"

#include <M5Unified.hpp>
#include <lgfx/utility/pgmspace.h>
#include <lgfx/v1/LGFXBase.hpp>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "splash.h"

static lgfx::LGFX_Device *s_display = nullptr;
static constexpr uint32_t FG = 0xF5972D;
static constexpr uint32_t BG = 0x000000;

static uint16_t
rgb888_to_565(uint32_t color) {
  return (uint16_t)(((color >> 8) & 0xF800) | ((color >> 5) & 0x07E0) |
                    ((color >> 3) & 0x001F));
}

static void
draw_boot_splash(void) {
  static constexpr int width = 240;
  static constexpr int height = 135;
  static constexpr int line_delay_ms = 1;

  if (!s_display || !epd_bitmap_allArray[0])
    return;

  const uint8_t *bitmap = (const uint8_t *)epd_bitmap_allArray[0];

  int row_bytes = (width + 7) / 8;
  int copy_w = width < s_display->width() ? width : s_display->width();
  int copy_h = height < s_display->height() ? height : s_display->height();
  int src_x = width > s_display->width() ? (width - s_display->width()) / 2 : 0;
  int src_y =
    height > s_display->height() ? (height - s_display->height()) / 2 : 0;
  int dst_x = s_display->width() > width ? (s_display->width() - width) / 2 : 0;
  int dst_y =
    s_display->height() > height ? (s_display->height() - height) / 2 : 0;

  if (copy_w <= 0 || copy_h <= 0 || (src_x % 8) != 0)
    return;

  TickType_t delay = pdMS_TO_TICKS(line_delay_ms);

  if (delay == 0)
    delay = 1;

  for (int y = 0; y < copy_h; ++y) {
    const uint8_t *row = bitmap + (src_y + y) * row_bytes + src_x / 8;

    s_display->startWrite();

    s_display->drawBitmap(
      dst_x,
      dst_y + y,
      row,
      copy_w,
      1,
      rgb888_to_565(FG),
      rgb888_to_565(BG)
    );

    s_display->endWrite();

    vTaskDelay(delay);
  }

  vTaskDelay(pdMS_TO_TICKS(500));
}

extern "C" void
area512_display_init(void) {
  if (s_display)
    return;

  auto config = M5.config();
  M5.begin(config);

  s_display = &M5.Display;

  s_display->setBrightness(90);
  s_display->fillScreen(BG);

  draw_boot_splash();
}

extern "C" void *
area512_display_device(void) {
  return s_display;
}
