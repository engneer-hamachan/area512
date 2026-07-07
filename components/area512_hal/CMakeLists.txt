idf_component_register(
  SRCS
    console/input.cpp
    console/keyboard.cpp
    console/getch.cpp
    console/system_metrics.cpp
    console/vm_stats.c
    gfx/area512_display.cpp
    gfx/area512_gfx.cpp
    sd/area512_data_path.c
    sd/area512_sd.c
    sd/area512_seed.c
  INCLUDE_DIRS
    include
    console
    gfx
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

set(AREA512_SEED_DIR ${COMPONENT_DIR}/../../storage)
set(AREA512_SEED_TAR ${CMAKE_BINARY_DIR}/area512_seed.tar)
file(GLOB AREA512_SEED_TOP_ENTRIES RELATIVE ${AREA512_SEED_DIR} ${AREA512_SEED_DIR}/*)
file(GLOB_RECURSE AREA512_SEED_FILES ${AREA512_SEED_DIR}/*)

add_custom_command(
  OUTPUT ${AREA512_SEED_TAR}
  COMMAND ${CMAKE_COMMAND} -E tar cf ${AREA512_SEED_TAR} --format=gnutar -- ${AREA512_SEED_TOP_ENTRIES}
  WORKING_DIRECTORY ${AREA512_SEED_DIR}
  DEPENDS ${AREA512_SEED_FILES}
  VERBATIM
)

target_add_binary_data(${COMPONENT_LIB} ${AREA512_SEED_TAR} BINARY DEPENDS ${AREA512_SEED_TAR})
