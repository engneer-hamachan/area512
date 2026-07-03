idf_component_register(
  SRCS "main.c"
  REQUIRES area512
  PRIV_REQUIRES spi_flash
  INCLUDE_DIRS ""
)

set(image ../storage)
littlefs_create_partition_image(storage ${image} FLASH_IN_PROJECT)
