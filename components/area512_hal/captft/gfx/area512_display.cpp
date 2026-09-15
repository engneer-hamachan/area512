#include "area512_display.h"

#include "area512_hal.h"

#include <M5Unified.hpp>
#include <lgfx/utility/pgmspace.h>
#include <lgfx/v1/LGFXBase.hpp>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "splash.h"

static constexpr int PANEL_WIDTH = 240;
static constexpr int PANEL_HEIGHT = 320;
static constexpr int PANEL_ROTATION = 1;
static constexpr int SPLASH_LINE_DELAY_MS = 1;
static constexpr int SPLASH_HOLD_MS = 500;

static lgfx::LGFX_Device *s_display = nullptr;

static void
set_panel_size_and_rotation(lgfx::LGFX_Device *display) {
  lgfx::v1::Panel_Device *panel = display->panel();

  if (panel == nullptr)
    return;

  auto config = panel->config();

  config.panel_width = PANEL_WIDTH;
  config.panel_height = PANEL_HEIGHT;
  config.offset_x = 0;
  config.offset_y = 0;

  panel->config(config);
  display->setRotation(PANEL_ROTATION);
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

  s_display = &M5.Display;

  set_panel_size_and_rotation(s_display);

  s_display->setBrightness(90);
  s_display->fillScreen(area512_theme_background_color());
}

extern "C" void *
area512_display_device(void) {
  return s_display;
}
