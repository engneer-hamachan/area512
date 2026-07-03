// Firmware entry: brings up NVS, the mruby/c VM heap, and the main Ruby
// task, then runs the VM. Called once from the ESP-IDF app_main.
#include "picoruby.h"
#include "sdkconfig.h"
#include <inttypes.h>
#include <nvs_flash.h>
#if defined(CONFIG_ESP_CONSOLE_SECONDARY_USB_SERIAL_JTAG) ||                   \
  defined(CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG)
#include "driver/usb_serial_jtag_vfs.h"
#endif

#include <mrubyc.h>

#include "mrb/main_task.c"

// Cardputer has no PSRAM. The mruby/c VM heap (120KB) is sized to leave
// room for native allocations such as Sprites. See
// CARDPUTER_MINIMAL_RUNTIME.md.
#ifndef HEAP_SIZE
#define HEAP_SIZE (1024 * 120)
#endif

static uint8_t s_heap_pool[HEAP_SIZE];

void
setup(void) {
  // Pass bytes through verbatim (no CRLF translation) so binary data isn't
  // mangled; hosts that need CRLF convert on their side.
#if defined(CONFIG_ESP_CONSOLE_SECONDARY_USB_SERIAL_JTAG) ||                   \
  defined(CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG)
  usb_serial_jtag_vfs_set_tx_line_endings(ESP_LINE_ENDINGS_LF);
  usb_serial_jtag_vfs_set_rx_line_endings(ESP_LINE_ENDINGS_LF);
#endif

  esp_err_t ret = nvs_flash_init();

  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {

    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }

  ESP_ERROR_CHECK(ret);
}

void
area512_main(void) {
  setup();

  mrbc_init(s_heap_pool, HEAP_SIZE);

  mrbc_tcb *main_tcb = mrbc_create_task(main_task, 0);
  mrbc_set_task_name(main_tcb, "main_task");
  mrbc_vm *vm = &main_tcb->vm;

  picoruby_init_require(vm);

  mrbc_run();
}
