/*
 * SPDX-FileCopyrightText: 2015-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdbool.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "bootloader_init.h"
#include "bootloader_utility.h"
#include "bootloader_common.h"

static const char *TAG = "boot";

static int
load_partition_table_and_select_boot_index(bootloader_state_t *bootloader_state
) {
  if (!bootloader_utility_load_partition_table(bootloader_state)) {
    ESP_LOGE(TAG, "load partition table error!");
    return INVALID_INDEX;
  }

  if (esp_rom_get_reset_reason(0) == RESET_REASON_CHIP_POWER_ON &&
      bootloader_state->factory.offset != 0) {
    return FACTORY_INDEX;
  }

  return bootloader_utility_get_selected_boot_partition(bootloader_state);
}

void __attribute__((noreturn))
call_start_cpu0(void) {
  if (bootloader_init() != ESP_OK) {
    bootloader_reset();
  }

  bootloader_state_t bootloader_state = {0};
  int boot_index =
    load_partition_table_and_select_boot_index(&bootloader_state);

  if (boot_index == INVALID_INDEX) {
    bootloader_reset();
  }

  bootloader_utility_load_boot_image(&bootloader_state, boot_index);
}

#if CONFIG_LIBC_NEWLIB
struct _reent *
__getreent(void) {
  return _GLOBAL_REENT;
}
#endif
