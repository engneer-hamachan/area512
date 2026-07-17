// Forked from upstream picoruby-machine's esp32 port to route I/O through
// the Area512 console: keyboard read, display/input init, and yielding the
// input drain to a foreground session (e.g. Filer).
#include "hal.h"
#include "io-console.h"
#include "machine.h"
#include "ringbuffer.h"
#include <stdbool.h>

// Area512 console HAL hooks. Declared locally rather than via console.h: this
// file is on picoruby's include path, where mrubyc's own console.h would
// shadow ours and hide these symbols.
void area512_console_poll(void);
void area512_console_write(const void *buffer, int buffer_byte_count);
void area512_display_init(void);
void area512_input_init(void);

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include <freertos/FreeRTOS.h>
#include <freertos/portmacro.h>
#include <freertos/task.h>

#include "esp_sleep.h"
#include "esp_timer.h"
#include "hal/efuse_hal.h"
#include "rom/ets_sys.h"

#if CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG
#include "hal/usb_serial_jtag_ll.h"
#endif

#define ESP32_MSEC_PER_TICK (10)
#define ESP32_TIMER_UNIT_PER_SEC (1000000)

#ifdef MRBC_NO_TIMER
#error "MRBC_NO_TIMER is not supported"
#endif

static esp_timer_handle_t s_periodic_timer;
volatile int sigint_status = MACHINE_SIG_NONE;

// -----------------------------------------------------------------------------
// stdin RingBuffer
// -----------------------------------------------------------------------------

#ifndef PICORB_STDIN_BUFFER_SIZE
#define PICORB_STDIN_BUFFER_SIZE 1024
#endif

static uint8_t
  s_stdin_buffer_memory[sizeof(RingBuffer) + PICORB_STDIN_BUFFER_SIZE]
  __attribute__((aligned(4)));

static RingBuffer *s_stdin_ring_buffer = (RingBuffer *)s_stdin_buffer_memory;

bool
picorb_hal_stdin_push(uint8_t character) {
  if (!io_raw_q()) {
    if (character == 3) {
      sigint_status = MACHINE_SIGINT_RECEIVED;
      return true;
    }

    if (character == 26) {
      sigint_status = MACHINE_SIGTSTP_RECEIVED;
      return true;
    }
  }

  return RingBuffer_push(s_stdin_ring_buffer, character);
}

static void
stdin_reader_task(void *argument) {
  (void)argument;

  for (;;) {
    // Sole poller: being the only one keeps the stdin RingBuffer
    // single-producer, so it needs no lock.
    area512_console_poll();

#if CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG
    if (usb_serial_jtag_ll_rxfifo_data_available()) {
      uint8_t character;
      usb_serial_jtag_ll_read_rxfifo(&character, 1);
      picorb_hal_stdin_push(character);
    }
#endif

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

static void
advance_picorb_tick(void *argument) {
  (void)argument;

  picorb_tick();
}

void
picorb_hal_init(void) {
  esp_timer_create_args_t timer_create_args;

  timer_create_args.callback = &advance_picorb_tick;
  timer_create_args.arg = NULL;
  timer_create_args.dispatch_method = ESP_TIMER_TASK;
  timer_create_args.name = "mrbc_tick_timer";

  esp_timer_create(&timer_create_args, &s_periodic_timer);
  esp_timer_start_periodic(s_periodic_timer, MRBC_TICK_UNIT * 1000);

  // Order matters: display (M5.begin) must run before input (keyboard relies
  // on M5's I2C bus brought up by M5.begin).
  area512_display_init();
  area512_input_init();

  RingBuffer_init(s_stdin_ring_buffer, PICORB_STDIN_BUFFER_SIZE);

  xTaskCreate(
    stdin_reader_task,
    "stdin_reader",
    2048,
    NULL,
    tskIDLE_PRIORITY + 1,
    NULL
  );
}

void
picorb_hal_enable_irq(void) {
  portENABLE_INTERRUPTS();
}

void
picorb_hal_disable_irq(void) {
  portDISABLE_INTERRUPTS();
}

void
picorb_hal_idle_cpu(void) {
  vTaskDelay(1);
}

static FILE *
resolve_stream_for_fd(int fd) {
  return (fd == 1) ? stdout : stderr;
}

static void
flush_usb_serial_jtag(void) {
#if CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG
  usb_serial_jtag_ll_txfifo_flush();
#endif
}

int
picorb_hal_write(int fd, const void *buf, int nbytes) {
  FILE *stream = resolve_stream_for_fd(fd);

  for (int i = 0; i < nbytes; i++) {
    fputc(((char *)buf)[i], stream);
  }

  fflush(stream);

  flush_usb_serial_jtag();

  // Only stdout is mirrored to the screen; stderr stays serial-only.
  if (fd == 1)
    area512_console_write(buf, nbytes);

  return nbytes;
}

int
picorb_hal_flush(int fd) {
  FILE *stream = resolve_stream_for_fd(fd);
  int result = fflush(stream);

  flush_usb_serial_jtag();
  return result;
}

int
picorb_hal_read_available(void) {
  return (RingBuffer_data_size(s_stdin_ring_buffer) > 0) ? 1 : 0;
}

int
picorb_hal_getchar(void) {
  switch (sigint_status) {
  case MACHINE_SIGINT_RECEIVED:
    sigint_status = MACHINE_SIG_NONE;
    return 3;
  case MACHINE_SIGTSTP_RECEIVED:
    sigint_status = MACHINE_SIG_NONE;
    return 26;
  }

  uint8_t character;
  if (RingBuffer_pop(s_stdin_ring_buffer, &character)) {
    return (int)character;
  }

  return HAL_GETCHAR_NODATA;
}

void
picorb_hal_abort(const char *message) {
  if (message) {
    picorb_hal_write(1, message, strlen(message));
  }

  abort();
}

// -----------------------------------------------------------------------------
// USB
// -----------------------------------------------------------------------------

void
Machine_tud_task(void) {
  // Not required for ESP32.
}

bool
Machine_tud_mounted_q(void) {
  // Not required for ESP32.
  return false;
}

// -----------------------------------------------------------------------------
// RTC
// -----------------------------------------------------------------------------

// deep_sleep doesn't work yet.
void
Machine_deep_sleep(uint8_t gpio_pin, bool edge, bool high) {}

void
Machine_sleep(uint32_t seconds) {
  esp_sleep_enable_timer_wakeup(seconds * ESP32_TIMER_UNIT_PER_SEC);
  esp_light_sleep_start();
}

void
Machine_delay_ms(uint32_t ms) {
  vTaskDelay(ms / ESP32_MSEC_PER_TICK);
}

void
Machine_busy_wait_ms(uint32_t ms) {
  ets_delay_us(1000 * ms);
}

void
Machine_busy_wait_us(uint32_t us) {
  ets_delay_us(us);
}

bool
Machine_get_unique_id(char *id_string) {
  uint8_t mac_address[6];
  efuse_hal_get_mac(mac_address);
  sprintf(
    id_string,
    "%02X%02X%02X%02X%02X%02X",
    mac_address[0],
    mac_address[1],
    mac_address[2],
    mac_address[3],
    mac_address[4],
    mac_address[5]
  );
  return true;
}

uint32_t
Machine_stack_usage(void) {
  // Not implemented
  return 0;
}

bool
Machine_set_hwclock(const struct timespec *timespec) {
  clock_settime(CLOCK_REALTIME, timespec);
  return true;
}

bool
Machine_get_hwclock(struct timespec *timespec) {
  clock_gettime(CLOCK_REALTIME, timespec);
  return true;
}

void
Machine_exit(int status) {
  // Firmware has no process to exit to, so the status is meaningless here.
  (void)status;
}

uint64_t
Machine_uptime_us(void) {
  return (uint64_t)esp_timer_get_time();
}
