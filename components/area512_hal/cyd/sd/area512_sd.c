#include "driver/sdmmc_host.h"
#include "esp_err.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include <string.h>

#if defined(AREA512_CYD_ES3C28P)
#define AREA512_SD_PIN_CLK 38
#define AREA512_SD_PIN_CMD 40
#define AREA512_SD_PIN_D0 39
#define AREA512_SD_PIN_D1 41
#define AREA512_SD_PIN_D2 48
#define AREA512_SD_PIN_D3 47
#define AREA512_SD_BUS_WIDTH 4
#endif

static sdmmc_card_t *s_card = NULL;
static char s_mount_path[24] = {0};
static int s_mount_reference_count = 0;

int
area512_sd_mounted(void) {
  return s_card != NULL ? 1 : 0;
}

int
area512_sd_mount(const char *base_path) {
  if (s_card != NULL) {
    s_mount_reference_count++;
    return 0;
  }

  sdmmc_host_t host = SDMMC_HOST_DEFAULT();
  sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

  slot_config.width = AREA512_SD_BUS_WIDTH;
  slot_config.clk = AREA512_SD_PIN_CLK;
  slot_config.cmd = AREA512_SD_PIN_CMD;
  slot_config.d0 = AREA512_SD_PIN_D0;
  slot_config.d1 = AREA512_SD_PIN_D1;
  slot_config.d2 = AREA512_SD_PIN_D2;
  slot_config.d3 = AREA512_SD_PIN_D3;
  slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
    .format_if_mount_failed = false,
    .max_files = 2,
    .allocation_unit_size = 512,
  };

  if (
    esp_vfs_fat_sdmmc_mount(
      base_path,
      &host,
      &slot_config,
      &mount_config,
      &s_card
    ) != ESP_OK
  ) {

    s_card = NULL;
    return -2;
  }

  strncpy(s_mount_path, base_path, sizeof(s_mount_path) - 1);
  s_mount_path[sizeof(s_mount_path) - 1] = '\0';
  s_mount_reference_count = 1;

  return 0;
}

int
area512_sd_unmount(void) {
  if (s_card == NULL)
    return 0;

  if (s_mount_reference_count > 1) {
    s_mount_reference_count--;
    return 0;
  }

  esp_vfs_fat_sdcard_unmount(s_mount_path, s_card);
  s_card = NULL;
  s_mount_reference_count = 0;

  return 0;
}
