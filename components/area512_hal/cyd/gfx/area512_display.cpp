#include "area512_display.h"

#include "area512_hal.h"

#include <lgfx/utility/pgmspace.h>
#include <lgfx/v1/LGFXBase.hpp>
#include <lgfx/v1/platforms/esp32/Bus_SPI.hpp>
#include <lgfx/v1/platforms/esp32/Light_PWM.hpp>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "splash.h"

#if defined(AREA512_CYD_ES3C28P)
#include <lgfx/v1/touch/Touch_FT5x06.hpp>

#include "Panel_ILI9341.hpp"
#elif defined(AREA512_CYD_2432S028)
#include <lgfx/v1/panel/Panel_ST7789.hpp>

#include "Touch_XPT2046.hpp"
#endif

static constexpr int PANEL_WIDTH = 240;
static constexpr int PANEL_HEIGHT = 320;
static constexpr int PANEL_ROTATION = 1;
static constexpr int SPLASH_LINE_DELAY_MS = 1;
static constexpr int SPLASH_HOLD_MS = 500;

#if defined(AREA512_CYD_ES3C28P)
static constexpr int PIN_SCLK = 12;
static constexpr int PIN_MOSI = 11;
static constexpr int PIN_MISO = 13;
static constexpr int PIN_DC = 46;
static constexpr int PIN_CS = 10;
static constexpr int PIN_RST = -1;
static constexpr int PIN_BACKLIGHT = 45;
static constexpr uint32_t BACKLIGHT_PWM_FREQUENCY = 44100;
static constexpr bool PANEL_INVERT = true;

static constexpr int TOUCH_I2C_PORT = 1;
static constexpr int PIN_TOUCH_SDA = 16;
static constexpr int PIN_TOUCH_SCL = 15;
static constexpr int PIN_TOUCH_INT = 17;
static constexpr int PIN_TOUCH_RST = 18;
#elif defined(AREA512_CYD_2432S028)
static constexpr int PIN_SCLK = 14;
static constexpr int PIN_MOSI = 13;
static constexpr int PIN_MISO = 12;
static constexpr int PIN_DC = 2;
static constexpr int PIN_CS = 15;
static constexpr int PIN_RST = -1;
static constexpr int PIN_BACKLIGHT = 21;
static constexpr uint32_t BACKLIGHT_PWM_FREQUENCY = 44100;
static constexpr bool PANEL_INVERT = false;

static constexpr int PIN_TOUCH_SCLK = 25;
static constexpr int PIN_TOUCH_MOSI = 32;
static constexpr int PIN_TOUCH_MISO = 39;
static constexpr int PIN_TOUCH_CS = 33;
static constexpr int PIN_TOUCH_INT = 36;
static constexpr int TOUCH_RAW_X_MIN = 350;
static constexpr int TOUCH_RAW_X_MAX = 3700;
static constexpr int TOUCH_RAW_Y_MIN = 300;
static constexpr int TOUCH_RAW_Y_MAX = 3900;
#endif

static lgfx::Bus_SPI s_bus;
static lgfx::Light_PWM s_light;
#if defined(AREA512_CYD_ES3C28P)
static lgfx::Panel_ILI9341 s_panel;
static lgfx::Touch_FT5x06 s_touch;
#elif defined(AREA512_CYD_2432S028)
static lgfx::Panel_ST7789 s_panel;
static lgfx::Touch_XPT2046 s_touch;
#endif
static lgfx::LGFX_Device s_external_display;
static lgfx::LGFX_Device *s_display = nullptr;

static void
configure_panel(void) {
  auto bus_config = s_bus.config();

  bus_config.spi_host = SPI2_HOST;
  bus_config.pin_sclk = PIN_SCLK;
  bus_config.pin_mosi = PIN_MOSI;
  bus_config.pin_miso = PIN_MISO;
  bus_config.use_lock = true;
  bus_config.spi_mode = 0;
  bus_config.freq_write = 40000000;
  bus_config.spi_3wire = false;
  bus_config.pin_dc = PIN_DC;

  s_bus.config(bus_config);
  s_panel.setBus(&s_bus);

  auto config = s_panel.config();
  config.pin_cs = PIN_CS;
  config.pin_rst = PIN_RST;
  config.memory_width = PANEL_WIDTH;
  config.memory_height = PANEL_HEIGHT;
  config.panel_width = PANEL_WIDTH;
  config.panel_height = PANEL_HEIGHT;
  config.offset_x = 0;
  config.offset_y = 0;
  config.invert = PANEL_INVERT;
  config.rgb_order = false;
  config.readable = false;
  config.bus_shared = false;
  s_panel.config(config);

  auto light_config = s_light.config();

  light_config.pin_bl = PIN_BACKLIGHT;
  light_config.freq = BACKLIGHT_PWM_FREQUENCY;
  s_light.config(light_config);
  s_panel.setLight(&s_light);

  auto touch_config = s_touch.config();

#if defined(AREA512_CYD_ES3C28P)
  touch_config.x_min = 0;
  touch_config.x_max = PANEL_WIDTH - 1;
  touch_config.y_min = 0;
  touch_config.y_max = PANEL_HEIGHT - 1;
  touch_config.offset_rotation = 2;
  touch_config.pin_int = PIN_TOUCH_INT;
  touch_config.pin_rst = PIN_TOUCH_RST;
  touch_config.bus_shared = false;
  touch_config.i2c_port = TOUCH_I2C_PORT;
  touch_config.pin_sda = PIN_TOUCH_SDA;
  touch_config.pin_scl = PIN_TOUCH_SCL;
#elif defined(AREA512_CYD_2432S028)
  touch_config.x_min = TOUCH_RAW_X_MIN;
  touch_config.x_max = TOUCH_RAW_X_MAX;
  touch_config.y_min = TOUCH_RAW_Y_MIN;
  touch_config.y_max = TOUCH_RAW_Y_MAX;
  touch_config.offset_rotation = 4;
  touch_config.pin_int = PIN_TOUCH_INT;
  touch_config.bus_shared = false;
  touch_config.pin_sclk = PIN_TOUCH_SCLK;
  touch_config.pin_mosi = PIN_TOUCH_MOSI;
  touch_config.pin_miso = PIN_TOUCH_MISO;
  touch_config.pin_cs = PIN_TOUCH_CS;
#endif
  s_touch.config(touch_config);
  s_panel.setTouch(&s_touch);

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

extern "C" bool
area512_display_read_raw_touch_point(int *raw_x, int *raw_y) {
  lgfx::touch_point_t touch_point;

  if (!s_display || s_display->getTouchRaw(&touch_point, 1) == 0)
    return false;

  *raw_x = touch_point.x;
  *raw_y = touch_point.y;

  return true;
}

extern "C" void
area512_display_convert_screen_to_touch_calibration_point(
  int screen_x,
  int screen_y,
  int *calibration_x,
  int *calibration_y
) {

#if defined(AREA512_CYD_ES3C28P)
  *calibration_x = screen_y;
  *calibration_y = PANEL_HEIGHT - 1 - screen_x;
#elif defined(AREA512_CYD_2432S028)
  *calibration_x = screen_y;
  *calibration_y = screen_x;
#endif
}

extern "C" void
area512_display_read_touch_calibration_size(
  int *calibration_width,
  int *calibration_height
) {

  *calibration_width = PANEL_WIDTH;
  *calibration_height = PANEL_HEIGHT;
}

extern "C" void
area512_display_set_touch_calibration(
  const uint16_t calibration_values[AREA512_TOUCH_CALIBRATION_VALUE_COUNT]
) {

  if (!s_display)
    return;

  uint16_t mutable_calibration_values[AREA512_TOUCH_CALIBRATION_VALUE_COUNT];

  memcpy(
    mutable_calibration_values,
    calibration_values,
    sizeof(mutable_calibration_values)
  );

  s_display->setTouchCalibrate(mutable_calibration_values);
}

extern "C" void
area512_display_reset_touch_calibration(void) {
  s_panel.touchCalibrate();
}
