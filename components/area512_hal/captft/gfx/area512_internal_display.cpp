#include "area512_hal.h"
#include "area512_display.h"

#include <M5Unified.hpp>
#include <lgfx/v1/platforms/esp32/Bus_SPI.hpp>

#include <pthread.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static constexpr int WAKEUP_DELAY_MS = 130;
static pthread_mutex_t internal_display_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool is_internal_display_begun;

extern "C" int
area512_internal_display_begin(void) {
  if (!area512_display_device())
    return 0;

  pthread_mutex_lock(&internal_display_mutex);

  auto bus = static_cast<lgfx::Bus_SPI *>(M5.Display.panel()->bus());
  auto config = bus->config();

  config.dma_channel = 0;
  bus->config(config);

  if (!bus->init()) {
    pthread_mutex_unlock(&internal_display_mutex);
    return 0;
  }

  is_internal_display_begun = true;

  if (M5.Display.getBrightness() == 0) {
    M5.Display.wakeup();
    vTaskDelay(pdMS_TO_TICKS(WAKEUP_DELAY_MS));
    M5.Display.setBrightness(90);
  }

  M5.Display.setRotation(1);
  M5.Display.setTextSize(1);
  M5.Display.setTextWrap(false, false);
  M5.Display.setTextScroll(false);
  M5.Display.clearClipRect();

  return 1;
}

extern "C" void
area512_internal_display_end(void) {
  if (!is_internal_display_begun)
    return;

  M5.Display.waitDisplay();
  M5.Display.releaseBus();
  is_internal_display_begun = false;

  pthread_mutex_unlock(&internal_display_mutex);
}

extern "C" int
area512_internal_display_width(void) {
  return M5.Display.width();
}

extern "C" int
area512_internal_display_height(void) {
  return M5.Display.height();
}

extern "C" void
area512_internal_display_fill_rect(
  int x,
  int y,
  int width,
  int height,
  uint32_t color
) {
  if (!is_internal_display_begun)
    return;

  M5.Display.fillRect(x, y, width, height, color);
}

extern "C" void
area512_internal_display_text(
  int x,
  int y,
  const char *text,
  int byte_length,
  int font_size,
  uint32_t foreground,
  uint32_t background
) {
  if (!is_internal_display_begun || !text || byte_length <= 0)
    return;

  switch (font_size) {
  case 10:
    M5.Display.setFont(&lgfx::v1::fonts::efontJA_10);
    break;
  case 12:
    M5.Display.setFont(&lgfx::v1::fonts::efontJA_12);
    break;
  case 14:
    M5.Display.setFont(&lgfx::v1::fonts::efontJA_14);
    break;
  case 16:
    M5.Display.setFont(&lgfx::v1::fonts::efontJA_16);
    break;
  case 24:
    M5.Display.setFont(&lgfx::v1::fonts::efontJA_24);
    break;
  default:
    return;
  }

  M5.Display.setTextColor(foreground, background);
  M5.Display.setCursor(x, y);
  M5.Display.write(text, (size_t)byte_length);
}
