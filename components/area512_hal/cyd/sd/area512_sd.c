#include "esp_err.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include <string.h>

#if defined(AREA512_CYD_ES3C28P)
#include "driver/sdmmc_host.h"
#elif defined(AREA512_CYD_2432S028)
#include "driver/gpio.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "sd_protocol_defs.h"
#include <stdbool.h>
#endif

#if defined(AREA512_CYD_ES3C28P)
#define AREA512_SD_PIN_CLK 38
#define AREA512_SD_PIN_CMD 40
#define AREA512_SD_PIN_D0 39
#define AREA512_SD_PIN_D1 41
#define AREA512_SD_PIN_D2 48
#define AREA512_SD_PIN_D3 47
#define AREA512_SD_BUS_WIDTH 4
#elif defined(AREA512_CYD_2432S028)
#define AREA512_SD_PIN_CS 5
#define AREA512_SD_PIN_SCLK 18
#define AREA512_SD_PIN_MOSI 23
#define AREA512_SD_PIN_MISO 19
#define AREA512_SD_SPI_HOST SPI3_HOST
#define AREA512_SD_FREQ_KHZ 20000
#define AREA512_SD_MAX_TRANSFER_SIZE 4000
#endif

static sdmmc_card_t *s_card = NULL;
static char s_mount_path[24] = {0};
static int s_mount_reference_count = 0;
#if defined(AREA512_CYD_2432S028)
static bool s_spi_bus_initialized = false;
#endif

int
area512_sd_mounted(void) {
  return s_card != NULL ? 1 : 0;
}

#if defined(AREA512_CYD_2432S028)
// Answers the SDIO commands CMD52 and CMD5 with ESP_ERR_NOT_SUPPORTED without
// sending them to the card. sdmmc_card_init then treats the card as a memory
// card, the same as when the card itself rejects them. With them sent, the
// card answered the following CMD59 (CRC on) with "illegal command" and
// sdmmc_card_init failed with 0x106.
static esp_err_t
do_transaction_without_sdio_commands(int slot, sdmmc_command_t *command) {
  if (
    command->opcode == SD_IO_RW_DIRECT || command->opcode == SD_IO_SEND_OP_COND
  )
    return ESP_ERR_NOT_SUPPORTED;

  return sdspi_host_do_transaction(slot, command);
}
#endif

static esp_err_t
mount_sd_card_fat(
  const char *base_path,
  const esp_vfs_fat_sdmmc_mount_config_t *mount_config
) {

#if defined(AREA512_CYD_ES3C28P)
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

  return esp_vfs_fat_sdmmc_mount(
    base_path,
    &host,
    &slot_config,
    mount_config,
    &s_card
  );
#elif defined(AREA512_CYD_2432S028)
  if (!s_spi_bus_initialized) {
    spi_bus_config_t bus_config = {
      .mosi_io_num = AREA512_SD_PIN_MOSI,
      .miso_io_num = AREA512_SD_PIN_MISO,
      .sclk_io_num = AREA512_SD_PIN_SCLK,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .max_transfer_sz = AREA512_SD_MAX_TRANSFER_SIZE,
    };
    esp_err_t bus_result =
      spi_bus_initialize(AREA512_SD_SPI_HOST, &bus_config, SPI_DMA_CH_AUTO);

    if (bus_result != ESP_OK)
      return bus_result;

    s_spi_bus_initialized = true;
  }

  gpio_set_pull_mode(AREA512_SD_PIN_CS, GPIO_PULLUP_ONLY);

  sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
  slot_config.gpio_cs = AREA512_SD_PIN_CS;
  slot_config.host_id = AREA512_SD_SPI_HOST;

  sdmmc_host_t host = SDSPI_HOST_DEFAULT();
  host.slot = AREA512_SD_SPI_HOST;
  host.max_freq_khz = AREA512_SD_FREQ_KHZ;
  host.do_transaction = do_transaction_without_sdio_commands;

  return esp_vfs_fat_sdspi_mount(
    base_path,
    &host,
    &slot_config,
    mount_config,
    &s_card
  );
#endif
}

int
area512_sd_mount(const char *base_path) {
  if (s_card != NULL) {
    s_mount_reference_count++;
    return 0;
  }

  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
    .format_if_mount_failed = false,
    .max_files = 2,
    .allocation_unit_size = 512,
  };

  if (mount_sd_card_fat(base_path, &mount_config) != ESP_OK) {
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
