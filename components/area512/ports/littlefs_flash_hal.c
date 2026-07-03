// Forked from upstream picoruby-littlefs's esp32 flash HAL to keep the
// Cardputer flash layout (768 blocks = 3 MB; upstream is 256).
#include <string.h>

#include "esp_flash.h"

#include "lfs.h"
#include "littlefs.h"

#if !defined(LFS_FLASH_TARGET_OFFSET)
#define LFS_FLASH_TARGET_OFFSET 0x00210000
#endif

#define LFS_FLASH_BLOCK_SIZE 4096 // == SPI_FLASH_SEC_SIZE
// Must match the storage partition in partitions.csv (3 MB / 4 KB = 768);
// too small fails mount with "Invalid block count".
#define LFS_FLASH_BLOCK_COUNT 768 // 3 MB
#define LFS_FLASH_PAGE_SIZE 256

static int
lfs_flash_read(
  const struct lfs_config *config,
  lfs_block_t block,
  lfs_off_t offset,
  void *buffer,
  lfs_size_t size
) {

  uint32_t address =
    LFS_FLASH_TARGET_OFFSET + block * config->block_size + offset;

  esp_err_t result =
    esp_flash_read(esp_flash_default_chip, buffer, address, size);

  if (result != ESP_OK) {
    return LFS_ERR_IO;
  }

  return LFS_ERR_OK;
}

static int
lfs_flash_program(
  const struct lfs_config *config,
  lfs_block_t block,
  lfs_off_t offset,
  const void *buffer,
  lfs_size_t size
) {

  uint32_t address =
    LFS_FLASH_TARGET_OFFSET + block * config->block_size + offset;

  esp_err_t result =
    esp_flash_write(esp_flash_default_chip, buffer, address, size);

  if (result != ESP_OK) {
    return LFS_ERR_IO;
  }

  return LFS_ERR_OK;
}

static int
lfs_flash_erase(const struct lfs_config *config, lfs_block_t block) {
  uint32_t address = LFS_FLASH_TARGET_OFFSET + block * config->block_size;

  esp_err_t result =
    esp_flash_erase_region(esp_flash_default_chip, address, config->block_size);

  if (result != ESP_OK) {
    return LFS_ERR_IO;
  }

  return LFS_ERR_OK;
}

static int
lfs_flash_sync(const struct lfs_config *config) {
  (void)config;
  return LFS_ERR_OK;
}

void
littlefs_hal_init_config(struct lfs_config *config) {
  memset(config, 0, sizeof(struct lfs_config));
  config->read = lfs_flash_read;
  config->prog = lfs_flash_program;
  config->erase = lfs_flash_erase;
  config->sync = lfs_flash_sync;

  config->read_size = LFS_FLASH_PAGE_SIZE;
  config->prog_size = LFS_FLASH_PAGE_SIZE;
  config->block_size = LFS_FLASH_BLOCK_SIZE;
  config->block_count = LFS_FLASH_BLOCK_COUNT;
  config->block_cycles = 500;
  config->cache_size = LFS_FLASH_PAGE_SIZE;
  config->lookahead_size = 16;
}

void
littlefs_hal_erase_all(void) {
  esp_flash_erase_region(
    esp_flash_default_chip,
    (uint32_t)LFS_FLASH_TARGET_OFFSET,
    (size_t)(LFS_FLASH_BLOCK_SIZE * LFS_FLASH_BLOCK_COUNT)
  );
}
