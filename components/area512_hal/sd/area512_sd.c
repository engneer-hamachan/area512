// microSD (SPI) mount HAL: mounts FAT at /sdcard via ESP-IDF sdspi +
// esp_vfs_fat so the picoruby-area512-sdfat gem can use libc fopen/opendir
// directly. Never auto-formats the card (format_if_mount_failed=false).

#include "driver/gpio.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include <string.h>

static const char *TAG = "area512_sd";

// Free internal DRAM; logged around each mount stage to track the ~44KB
// cost.
static unsigned int
free_dram(void) {
  return (unsigned int
  )heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
}

// Cardputer TF slot pins.
#define AREA512_SD_PIN_SCK 40
#define AREA512_SD_PIN_MISO 39
#define AREA512_SD_PIN_MOSI 14
#define AREA512_SD_PIN_CS 12
#define AREA512_SD_SPI_HOST SPI2_HOST // display owns SPI3_HOST
#define AREA512_SD_FREQ_KHZ 20000     // stable on Cardputer SD

static sdmmc_card_t *s_card = NULL;
static bool s_bus_inited = false;
static char s_base[24] = {0};
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

  esp_err_t ret;
  unsigned int dram0 = free_dram();
  ESP_LOGI(TAG, "[heap] before mount: internal free=%u", dram0);

  if (!s_bus_inited) {
    spi_bus_config_t bus_cfg = {
      .mosi_io_num = AREA512_SD_PIN_MOSI,
      .miso_io_num = AREA512_SD_PIN_MISO,
      .sclk_io_num = AREA512_SD_PIN_SCK,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .max_transfer_sz = 4000,
    };
    ret = spi_bus_initialize(AREA512_SD_SPI_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
      return -1;
    }
    s_bus_inited = true;
  }
  unsigned int dram1 = free_dram();
  ESP_LOGI(
    TAG,
    "[heap] after spi_bus_initialize: free=%u (used %d)",
    dram1,
    (int)dram0 - (int)dram1
  );

  // ESP-IDF sdspi sets no pull-ups; enable internal ones so floating
  // MISO/MOSI don't garble CMD8 (INVALID_RESPONSE 0x108).
  gpio_set_pull_mode(AREA512_SD_PIN_MISO, GPIO_PULLUP_ONLY);
  gpio_set_pull_mode(AREA512_SD_PIN_MOSI, GPIO_PULLUP_ONLY);
  gpio_set_pull_mode(AREA512_SD_PIN_SCK, GPIO_PULLUP_ONLY);
  gpio_set_pull_mode(AREA512_SD_PIN_CS, GPIO_PULLUP_ONLY);

  sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
  slot_config.gpio_cs = AREA512_SD_PIN_CS;
  slot_config.host_id = AREA512_SD_SPI_HOST;

  sdmmc_host_t host = SDSPI_HOST_DEFAULT();
  host.slot = AREA512_SD_SPI_HOST;
  host.max_freq_khz = AREA512_SD_FREQ_KHZ;

  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
    .format_if_mount_failed = false, // never auto-format
    .max_files = 2,                  // limit per-file cache
    .allocation_unit_size = 512,     // minimize mount work buffer
  };

  ret = esp_vfs_fat_sdspi_mount(
    base_path,
    &host,
    &slot_config,
    &mount_config,
    &s_card
  );
  if (ret != ESP_OK) {
    s_card = NULL;
    return -2;
  }
  unsigned int dram2 = free_dram();
  ESP_LOGI(
    TAG,
    "[heap] after fat mount: free=%u (mount used %d, total SD %d)",
    dram2,
    (int)dram1 - (int)dram2,
    (int)dram0 - (int)dram2
  );

  strncpy(s_base, base_path, sizeof(s_base) - 1);
  s_base[sizeof(s_base) - 1] = '\0';
  s_mount_reference_count = 1;
  return 0;
}

int
area512_sd_unmount(void) {
  if (s_card == NULL) {
    return 0;
  }
  if (s_mount_reference_count > 1) {
    s_mount_reference_count--;
    return 0;
  }
  esp_vfs_fat_sdcard_unmount(s_base, s_card);
  s_card = NULL;
  s_mount_reference_count = 0;
  return 0;
}
