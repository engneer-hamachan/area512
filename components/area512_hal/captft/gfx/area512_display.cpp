#include "area512_display.h"

#include "area512_hal.h"

#include <M5Unified.hpp>
#include <lgfx/utility/pgmspace.h>
#include <lgfx/v1/LGFXBase.hpp>
#include <lgfx/v1/panel/Panel_ST7789.hpp>
#include <lgfx/v1/platforms/esp32/Bus_SPI.hpp>

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "Panel_ILI9341.hpp"
#include "splash.h"

static constexpr int PANEL_WIDTH = 240;
static constexpr int PANEL_HEIGHT = 320;
static constexpr int PANEL_ROTATION = 1;
static constexpr int SPLASH_LINE_DELAY_MS = 1;
static constexpr int SPLASH_HOLD_MS = 500;

static lgfx::Bus_SPI s_bus;
#if defined(AREA512_CAPTFT_ILI9341)
static lgfx::Panel_ILI9341 s_panel;
#else
static lgfx::Panel_ST7789 s_panel;
#endif
static lgfx::LGFX_Device s_external_display;
static lgfx::LGFX_Device *s_display = nullptr;

static void
configure_panel(void) {
  auto bus_config = s_bus.config();

  // SPI2: shared by Cap TFT and the SD card (area512_sd.c).
  bus_config.spi_host = SPI2_HOST;
  bus_config.pin_sclk = 40;
  bus_config.pin_mosi = 14;
  bus_config.use_lock = true;

  // SPI2: SD card only. Cap TFT has no MISO, but SD mount reuses this init.
  bus_config.pin_miso = 39;

  // SPI2: Cap TFT only.
  bus_config.spi_mode = 0;
  bus_config.freq_write = 40000000;
  bus_config.spi_3wire = false;
  bus_config.pin_dc = 6;

  s_bus.config(bus_config);
  s_panel.setBus(&s_bus);

  // Cap TFT panel.
  auto config = s_panel.config();
  config.pin_cs = 5;
  config.pin_rst = 3;
  config.memory_width = PANEL_WIDTH;
  config.memory_height = PANEL_HEIGHT;
  config.panel_width = PANEL_WIDTH;
  config.panel_height = PANEL_HEIGHT;
  config.offset_x = 0;
  config.offset_y = 0;

#if defined(AREA512_CAPTFT_ILI9341)
  config.offset_rotation = 2;
#endif
  config.invert = false;
  config.rgb_order = false;
  config.readable = false;
  config.bus_shared = true;
  s_panel.config(config);
  s_external_display.setPanel(&s_panel);
}

extern "C" void
area512_display_draw_boot_splash(void) {
  if (!s_display || !epd_bitmap_allArray[0])
    return;

  area512_gfx_show_bitmap(
    epd_bitmap_allArray[0],
    SPLASH_LINE_DELAY_MS
  );

  vTaskDelay(pdMS_TO_TICKS(SPLASH_HOLD_MS));
}

extern "C" void
area512_display_init(void) {
  if (s_display)
    return;

  auto config = M5.config();
  M5.begin(config);

  // Keep M5 board/peripheral initialization, then release the internal display.
  M5.Display.setBrightness(0);
  M5.Display.sleep();
  M5.Display.waitDisplay();
  M5.Display.releaseBus();

  // Deselect the SD card before initializing the display on their shared bus.
  gpio_set_level(GPIO_NUM_12, 1);
  gpio_set_direction(GPIO_NUM_12, GPIO_MODE_OUTPUT);

  configure_panel();
  s_external_display.init();
  s_display = &s_external_display;
  s_display->setColorDepth(16);
  s_display->setRotation(PANEL_ROTATION);

  s_display->fillScreen(area512_theme_background_color());
}

extern "C" void *
area512_display_device(void) {
  return s_display;
}
