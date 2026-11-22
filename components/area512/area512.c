// Firmware entry: brings up NVS, the console and the Filer task, then
// returns. Called once from the ESP-IDF app_main.
#include "area512_arena.h"
#include "area512_hal.h"
#include "filer_shell.h"
#include "io-console.h"
#include "ports/machine_port.h"
#include "ports/tree_sitter_port.h"
#include "sdkconfig.h"
#include "watchdog.h"
#include <nvs_flash.h>
#if defined(CONFIG_ESP_CONSOLE_SECONDARY_USB_SERIAL_JTAG) ||                   \
  defined(CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG)
#include "driver/usb_serial_jtag_vfs.h"
#elif defined(CONFIG_ESP_CONSOLE_UART)
#include "driver/uart_vfs.h"
#endif

void
setup(void) {
  // Pass bytes through verbatim (no CRLF translation) so binary data isn't
  // mangled; hosts that need CRLF convert on their side.
#if defined(CONFIG_ESP_CONSOLE_SECONDARY_USB_SERIAL_JTAG) ||                   \
  defined(CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG)
  usb_serial_jtag_vfs_set_tx_line_endings(ESP_LINE_ENDINGS_LF);
  usb_serial_jtag_vfs_set_rx_line_endings(ESP_LINE_ENDINGS_LF);
#elif defined(CONFIG_ESP_CONSOLE_UART)
  uart_vfs_dev_port_set_tx_line_endings(
    CONFIG_ESP_CONSOLE_UART_NUM,
    ESP_LINE_ENDINGS_LF
  );
  uart_vfs_dev_port_set_rx_line_endings(
    CONFIG_ESP_CONSOLE_UART_NUM,
    ESP_LINE_ENDINGS_LF
  );
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
#if defined(AREA512_CYD_2432S028)
  area512_allocate_arena();
#endif
  area512_restore_factory_boot_partition();

  area512_start_console();

  area512_route_tree_sitter_allocations_to_mrubyc_pool();
  area512_register_tree_sitter_parse_budget();

  Watchdog_disable();
  io_echo_eq(false);

  area512_start_filer();
}
