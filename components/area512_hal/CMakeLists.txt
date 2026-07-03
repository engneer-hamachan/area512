# Skip the early-expansion pass (idf_build_* are undefined there).
if(NOT CMAKE_BUILD_EARLY_EXPANSION)
  set(_AREA512_GLOBAL_DEFS
    # Must match the storage offset in partitions.csv.
    -DLFS_FLASH_TARGET_OFFSET=0x410000
  )
  idf_build_set_property(COMPILE_OPTIONS "${_AREA512_GLOBAL_DEFS}" APPEND)
endif()

set(PURE_PICORUBY ${CMAKE_CURRENT_LIST_DIR}/../../R2P2-ESP32/components/picoruby-esp32/picoruby)

idf_component_register(
  SRCS
    console/input.cpp
    console/keyboard.cpp
    console/getch.cpp
    console/system_metrics.cpp
    console/vm_stats.c
    gfx/area512_display.cpp
    gfx/area512_gfx.cpp
    sd/area512_sd.c
  INCLUDE_DIRS
    include
    console
    gfx
  PRIV_INCLUDE_DIRS
    ${PURE_PICORUBY}/mrbgems/picoruby-littlefs/include
    ${PURE_PICORUBY}/mrbgems/picoruby-littlefs/lib/littlefs
  REQUIRES
    M5Unified
    m5gfx
  PRIV_REQUIRES
    esp_driver_gpio
    esp_driver_spi
    esp_driver_sdspi
    sdmmc
    fatfs
    spi_flash
    nvs_flash
)

# Define the VM macro here too; area512's add_definitions() doesn't reach
# this component, and without it the stats fall through #else and return 0.
set_source_files_properties(
  console/vm_stats.c
  PROPERTIES COMPILE_DEFINITIONS "PICORB_VM_MRUBYC"
)
