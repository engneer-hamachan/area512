#pragma once

#include <driver/gpio.h>
#include <esp_rom_sys.h>
#include <freertos/FreeRTOS.h>
#include <lgfx/v1/Touch.hpp>

#include "area512_hal.h"

namespace lgfx {
inline namespace v1 {

struct Touch_XPT2046 : public ITouch {
  bool init(void) override {
    if (_inited)
      return true;

    gpio_config_t output_pin_config = {};

    output_pin_config.pin_bit_mask =
      (1ULL << _cfg.pin_sclk) | (1ULL << _cfg.pin_mosi) | (1ULL << _cfg.pin_cs);

    output_pin_config.mode = GPIO_MODE_OUTPUT;

    if (gpio_config(&output_pin_config) != ESP_OK)
      return false;

    gpio_config_t miso_pin_config = {};

    miso_pin_config.pin_bit_mask = 1ULL << _cfg.pin_miso;
    miso_pin_config.mode = GPIO_MODE_INPUT;

    if (gpio_config(&miso_pin_config) != ESP_OK)
      return false;

    gpio_set_level((gpio_num_t)_cfg.pin_cs, 1);
    gpio_set_level((gpio_num_t)_cfg.pin_sclk, 0);

    if (_cfg.pin_int >= 0) {
      gpio_config_t interrupt_pin_config = {};

      interrupt_pin_config.pin_bit_mask = 1ULL << _cfg.pin_int;
      interrupt_pin_config.mode = GPIO_MODE_INPUT;

      if (gpio_config(&interrupt_pin_config) != ESP_OK)
        return false;
    }

    _inited = true;

    return true;
  }

  void wakeup(void) override {
  }

  void sleep(void) override {
  }

  uint_fast8_t getTouchRaw(
    touch_point_t *touch_point,
    uint_fast8_t count
  ) override {
    if (!_inited || count == 0)
      return 0;

    if (_cfg.pin_int >= 0 && gpio_get_level((gpio_num_t)_cfg.pin_int) != 0)
      return 0;

    if (read_pressure_and_enable_pen_irq() < MIN_TOUCH_PRESSURE)
      return 0;

    int raw_x_samples[RAW_POSITION_SAMPLE_COUNT];
    int raw_y_samples[RAW_POSITION_SAMPLE_COUNT];

    for (int i = 0; i < RAW_POSITION_SAMPLE_COUNT; i++) {
      raw_x_samples[i] = read_channel(READ_X_COMMAND);
      raw_y_samples[i] = read_channel(READ_Y_COMMAND);
    }

    int pressure = read_pressure_and_enable_pen_irq();

    if (pressure < MIN_TOUCH_PRESSURE)
      return 0;

    touch_point->x =
      area512_sort_and_find_median(raw_x_samples, RAW_POSITION_SAMPLE_COUNT);

    touch_point->y =
      area512_sort_and_find_median(raw_y_samples, RAW_POSITION_SAMPLE_COUNT);

    touch_point->size = pressure;
    touch_point->id = 0;

    return 1;
  }

private:
  static constexpr uint8_t READ_X_COMMAND = 0xD1;
  static constexpr uint8_t READ_Y_COMMAND = 0x91;
  static constexpr uint8_t READ_Z1_COMMAND = 0xB1;
  static constexpr uint8_t READ_Z2_COMMAND = 0xC1;
  static constexpr uint8_t READ_Y_AND_ENABLE_IRQ_COMMAND = 0x90;
  static constexpr int MAX_CHANNEL_VALUE = 4095;
  static constexpr int MIN_TOUCH_PRESSURE = 10;
  static constexpr int RAW_POSITION_SAMPLE_COUNT = 5;
  static constexpr int PRESSURE_SAMPLE_COUNT = 3;

  static constexpr uint32_t CLOCK_HALF_PERIOD_US = 1;

  uint8_t transfer_byte(uint8_t transmit_byte) {
    uint8_t receive_byte = 0;

    for (int bit = 7; bit >= 0; bit--) {
      gpio_set_level(
        (gpio_num_t)_cfg.pin_mosi,
        (transmit_byte >> bit) & 1
      );

      esp_rom_delay_us(CLOCK_HALF_PERIOD_US);
      gpio_set_level((gpio_num_t)_cfg.pin_sclk, 1);

      receive_byte =
        (receive_byte << 1) |
        gpio_get_level((gpio_num_t)_cfg.pin_miso);

      esp_rom_delay_us(CLOCK_HALF_PERIOD_US);

      gpio_set_level((gpio_num_t)_cfg.pin_sclk, 0);
    }

    return receive_byte;
  }

  portMUX_TYPE transfer_spinlock = portMUX_INITIALIZER_UNLOCKED;

  int read_channel(uint8_t command) {
    taskENTER_CRITICAL(&transfer_spinlock);

    gpio_set_level((gpio_num_t)_cfg.pin_cs, 0);
    transfer_byte(command);
    uint8_t high_byte = transfer_byte(0);
    uint8_t low_byte = transfer_byte(0);
    gpio_set_level((gpio_num_t)_cfg.pin_cs, 1);

    taskEXIT_CRITICAL(&transfer_spinlock);

    return ((high_byte << 8) | low_byte) >> 3;
  }

  int read_median_pressure(void) {
    int pressure_samples[PRESSURE_SAMPLE_COUNT];

    for (int i = 0; i < PRESSURE_SAMPLE_COUNT; i++) {
      int z1 = read_channel(READ_Z1_COMMAND);
      int z2 = read_channel(READ_Z2_COMMAND);

      pressure_samples[i] = z1 + MAX_CHANNEL_VALUE - z2;
    }

    return area512_sort_and_find_median(pressure_samples, PRESSURE_SAMPLE_COUNT);
  }

  int read_pressure_and_enable_pen_irq(void) {
    if (!_inited)
      return 0;

    int pressure = read_median_pressure();

    read_channel(READ_Y_AND_ENABLE_IRQ_COMMAND);

    return pressure;
  }
};

} // namespace v1
} // namespace lgfx
