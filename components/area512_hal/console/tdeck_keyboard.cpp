#include "console_internal.h"

#include <utility/I2C_Class.hpp>
#include <string.h>

#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "keyboard";
static constexpr uint8_t TDECK_KEYBOARD_ADDR = 0x55;
static constexpr uint32_t TDECK_KEYBOARD_FREQ = 200000;
static constexpr int TDECK_KEYBOARD_SDA = 18;
static constexpr int TDECK_KEYBOARD_SCL = 8;
static constexpr uint8_t LILYGO_KB_MODE_RAW_CMD = 0x03;
static constexpr int TDECK_KEYBOARD_COLUMNS = 5;
static constexpr int TDECK_KEYBOARD_ROWS = 7;
static constexpr int TDECK_REPEAT_DELAY_MS = 450;
static constexpr int TDECK_REPEAT_INTERVAL_MS = 55;

static m5::I2C_Class tdeck_keyboard_i2c;
static uint8_t s_tdeck_key_down[TDECK_KEYBOARD_COLUMNS] = {};
static int s_tdeck_repeat_column = -1;
static int s_tdeck_repeat_row = -1;
static TickType_t s_tdeck_repeat_next_tick = 0;

// GPIO order and transitions follow LilyGO's UnitTest mouse_read().
static const gpio_num_t TDECK_TRACKBALL_PINS[] = {
  GPIO_NUM_2, GPIO_NUM_3, GPIO_NUM_1, GPIO_NUM_15, GPIO_NUM_0,
};

static const char *const TDECK_TRACKBALL_SEQUENCES[] = {
  "\x1b[C", "\x1b[A", "\x1b[D", "\x1b[B",
};

static int s_tdeck_trackball_level[5] = {};
static int s_tdeck_trackball_count[4] = {};
static bool s_tdeck_trackball_pressed = false;
static TickType_t s_tdeck_trackball_changed_tick = 0;
static constexpr int TDECK_TRACKBALL_DEBOUNCE_MS = 20;

static bool
tdeck_trackball_init(void) {
  gpio_config_t config = {};

  for (int i = 0; i < 5; i++)
    config.pin_bit_mask |= 1ULL << TDECK_TRACKBALL_PINS[i];

  config.mode = GPIO_MODE_INPUT;
  config.pull_up_en = GPIO_PULLUP_ENABLE;
  config.pull_down_en = GPIO_PULLDOWN_DISABLE;
  config.intr_type = GPIO_INTR_DISABLE;

  if (gpio_config(&config) != ESP_OK) {
    ESP_LOGW(TAG, "T-Deck trackball GPIO init failed");
    return false;
  }

  for (int i = 0; i < 5; i++)
    s_tdeck_trackball_level[i] =
      gpio_get_level(TDECK_TRACKBALL_PINS[i]);

  for (int i = 0; i < 4; i++)
    s_tdeck_trackball_count[i] = 0;

  s_tdeck_trackball_pressed = s_tdeck_trackball_level[4] == 0;
  s_tdeck_trackball_changed_tick = xTaskGetTickCount();

  return true;
}

static void
tdeck_trackball_poll(void) {
  for (int i = 0; i < 4; i++) {
    int level = gpio_get_level(TDECK_TRACKBALL_PINS[i]);

    if (level != s_tdeck_trackball_level[i]) {
      s_tdeck_trackball_level[i] = level;

      if (++s_tdeck_trackball_count[i] >= 7) {
        s_tdeck_trackball_count[i] = 0;
        area512_console_stdin_push_bytes(TDECK_TRACKBALL_SEQUENCES[i]);
      }
    }
  }

  TickType_t now = xTaskGetTickCount();
  int level = gpio_get_level(TDECK_TRACKBALL_PINS[4]);

  if (level != s_tdeck_trackball_level[4]) {
    s_tdeck_trackball_level[4] = level;
    s_tdeck_trackball_changed_tick = now;
  }

  bool pressed = level == 0;
  if (
    pressed != s_tdeck_trackball_pressed &&
    now - s_tdeck_trackball_changed_tick >=
      pdMS_TO_TICKS(TDECK_TRACKBALL_DEBOUNCE_MS)
  ) {

    s_tdeck_trackball_pressed = pressed;

    if (pressed)
      area512_console_stdin_push('\r');
  }
}

// Column/row positions from LilyGO's Keyboard_ESP32C3 firmware.
static const uint8_t TDECK_KEYS[TDECK_KEYBOARD_COLUMNS][TDECK_KEYBOARD_ROWS] = {
  {'q', 'w', 0, 'a', 0, ' ', '='},
  {'e', 's', 'd', 'p', 'x', 'z', 0},
  {'r', 'g', 't', 0, 'v', 'c', 'f'},
  {'u', 'h', 'y', '\r', 'b', 'n', 'j'},
  {'o', 'l', 'i', '\b', '$', 'm', 'k'},
};

static const uint8_t TDECK_SYMBOL_KEYS[TDECK_KEYBOARD_COLUMNS]
                                      [TDECK_KEYBOARD_ROWS] = {
                                        {'#', '1', 0, '*', 0, 0, '0'},
                                        {'2', '4', '5', '@', '8', '7', 0},
                                        {'3', '/', '(', 0, '?', '9', '6'},
                                        {'_', ':', ')', '\r', '!', ',', ';'},
                                        {'+', '"', '-', '\b', 0, '.', '\''},
};

static void
tdeck_clear_repeat_tracking(void) {
  s_tdeck_repeat_column = -1;
  s_tdeck_repeat_row = -1;
  s_tdeck_repeat_next_tick = 0;
}

static void
tdeck_set_repeat_key(int column, int row) {
  if (TDECK_KEYS[column][row] == 0 && TDECK_SYMBOL_KEYS[column][row] == 0)
    return;

  s_tdeck_repeat_column = column;
  s_tdeck_repeat_row = row;
  s_tdeck_repeat_next_tick =
    xTaskGetTickCount() + pdMS_TO_TICKS(TDECK_REPEAT_DELAY_MS);
}

bool
keyboard_init(void) {
  if (!tdeck_trackball_init())
    return false;

  if (
    !tdeck_keyboard_i2c.begin(
      I2C_NUM_1,
      TDECK_KEYBOARD_SDA,
      TDECK_KEYBOARD_SCL)
  ) {

    ESP_LOGW(TAG, "T-Deck keyboard I2C init failed");

    return false;
  }

  if (
    !tdeck_keyboard_i2c.start(
      TDECK_KEYBOARD_ADDR,
      false,
      TDECK_KEYBOARD_FREQ)
  ) {

    return false;
  }

  bool ok = tdeck_keyboard_i2c.write(LILYGO_KB_MODE_RAW_CMD);
  bool stopped = tdeck_keyboard_i2c.stop();

  memset(s_tdeck_key_down, 0, sizeof(s_tdeck_key_down));

  tdeck_clear_repeat_tracking();

  return ok && stopped;
}

static uint8_t
tdeck_special_key_to_char(uint8_t key_event) {
  switch (key_event) {
  case 15:
    return '[';
  case 26:
    return ']';
  case 31:
    return '{';
  case 1:
    return '}';
  case 23:
    return '<';
  case 24:
    return '>';
  case 3:
    return '=';
  case 224:
    return '|';
  case 0x19:
    return 0x03; // sym+shift+c -> Ctrl-C
  case 0x15:
    return 0x04; // sym+shift+d -> Ctrl-D
  case 0x0C:
    return 0x0E; // sym+shift+n -> Ctrl-N
  default:
    return key_event;
  }
}

static uint8_t
tdeck_ctrl_letter_to_char(uint8_t letter) {
  switch (letter) {
  case 'j':
    return 0x1B;
  case 't':
    return '\t';
  case 'z':
    return '<';
  case 'x':
    return '>';
  default:
    return (uint8_t)(letter - 'a' + 1);
  }
}

static void
tdeck_handle_key(int column, int row, bool ctrl, bool shift, bool symbol) {
  uint8_t character = TDECK_KEYS[column][row];

  if (character == 0 && TDECK_SYMBOL_KEYS[column][row] == 0)
    return;

  if (character == '\r' || character == '\b') {
    area512_console_stdin_push(character);
    return;
  }

  if (ctrl && character >= 'a' && character <= 'z') {
    area512_console_stdin_push(
      tdeck_ctrl_letter_to_char(character)
    );

    return;
  }

  if (symbol)
    character = TDECK_SYMBOL_KEYS[column][row];

  if (character == 0 && !shift)
    return;

  if (shift)
    character = (uint8_t)(character - 32);

  if (symbol && shift)
    character = tdeck_special_key_to_char(character);

  if (ctrl) {
    if (character >= '@' && character <= '_')
      character &= 0x1F;
    else if (character == ' ')
      character = 0;
    else if (character == '?')
      character = 0x7F;
    else
      return;
  }

  area512_console_stdin_push(character);
}

void
keyboard_poll(void) {
  tdeck_trackball_poll();

  if (
    !tdeck_keyboard_i2c.start(
      TDECK_KEYBOARD_ADDR,
      true,
      TDECK_KEYBOARD_FREQ)
  ) {

    tdeck_clear_repeat_tracking();

    return;
  }

  uint8_t keys[TDECK_KEYBOARD_COLUMNS] = {};

  bool ok =
    tdeck_keyboard_i2c.read(keys, sizeof(keys), true);

  bool stopped = tdeck_keyboard_i2c.stop();

  if (!ok || !stopped) {
    tdeck_clear_repeat_tracking();
    return;
  }

  for (int column = 0; column < TDECK_KEYBOARD_COLUMNS; column++) {
    if (keys[column] & 0x80) {
      tdeck_clear_repeat_tracking();
      return;
    }
  }

  bool ctrl = (keys[0] & (1 << 4)) != 0; // Alt
  bool shift = (keys[1] & (1 << 6)) != 0 || (keys[2] & (1 << 3)) != 0;
  bool symbol = (keys[0] & (1 << 2)) != 0;

  if (
    s_tdeck_repeat_column >= 0 &&
    !(keys[s_tdeck_repeat_column] & (1 << s_tdeck_repeat_row))
  ) {

    tdeck_clear_repeat_tracking();
  }

  for (int row = 0; row < TDECK_KEYBOARD_ROWS; row++) {
    for (int column = 0; column < TDECK_KEYBOARD_COLUMNS; column++) {
      uint8_t pressed = keys[column] & (uint8_t)~s_tdeck_key_down[column];

      if (pressed & (1 << row)) {
        tdeck_handle_key(column, row, ctrl, shift, symbol);
        tdeck_set_repeat_key(column, row);
      }
    }
  }

  memcpy(s_tdeck_key_down, keys, sizeof(keys));

  if (s_tdeck_repeat_column < 0) {
    for (
      int row = 0;
      row < TDECK_KEYBOARD_ROWS && s_tdeck_repeat_column < 0;
      row++
    ) {
      for (
        int column = 0;
        column < TDECK_KEYBOARD_COLUMNS && s_tdeck_repeat_column < 0;
        column++
      ) {
        if (keys[column] & (1 << row))
          tdeck_set_repeat_key(column, row);
      }
    }
  }

  TickType_t now = xTaskGetTickCount();

  if (
    s_tdeck_repeat_column >= 0 &&
    (int32_t)(now - s_tdeck_repeat_next_tick) >= 0
  ) {

    tdeck_handle_key(
      s_tdeck_repeat_column,
      s_tdeck_repeat_row,
      ctrl,
      shift,
      symbol
    );

    s_tdeck_repeat_next_tick = now + pdMS_TO_TICKS(TDECK_REPEAT_INTERVAL_MS);
  }
}

void
area512_console_feed_ime(uint8_t byte) {
  area512_console_stdin_push(byte);
}

void
area512_console_flush_ime(void) {
}

void
area512_console_toggle_ime(void) {
}
