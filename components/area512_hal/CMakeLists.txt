set(AREA512_DISPLAY_DIR "")
if(AREA512_EXT_DISPLAY)
  set(AREA512_DISPLAY_DIR "captft/")
endif()

idf_component_register(
  SRCS
    console/input.cpp
    console/keyboard.cpp
    console/getch.cpp
    ${AREA512_DISPLAY_DIR}console/output.c
    console/system_metrics.cpp
    console/vm_stats.c
    ${AREA512_DISPLAY_DIR}gfx/area512_display.cpp
    ${AREA512_DISPLAY_DIR}gfx/area512_gfx.cpp
    sd/area512_data_path.c
    sd/area512_sd.c
    sd/area512_seed.c
    theme/area512_theme.c
    ui/area512_default_ui.c
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

if(AREA512_EXT_DISPLAY)
  target_sources(${COMPONENT_LIB} PRIVATE
    captft/gfx/area512_internal_display.cpp
  )
  target_compile_definitions(${COMPONENT_LIB} PUBLIC AREA512_EXT_DISPLAY)

  if(AREA512_CAPTFT_ILI9341)
    set_source_files_properties(
      captft/gfx/area512_display.cpp
      PROPERTIES COMPILE_DEFINITIONS "AREA512_CAPTFT_ILI9341"
    )
  endif()
endif()

if(AREA512_CARDPUTER_V11)
  set_source_files_properties(
    console/keyboard.cpp
    PROPERTIES COMPILE_DEFINITIONS "AREA512_CARDPUTER_V11"
  )
endif()

# Define the VM macro here too; area512's add_definitions() doesn't reach
# this component, and without it the stats fall through #else and return 0.
set_source_files_properties(
  console/vm_stats.c
  PROPERTIES COMPILE_DEFINITIONS "PICORB_VM_MRUBYC"
)

set(AREA512_SEED_DIR ${COMPONENT_DIR}/../../storage)
set(AREA512_SEED_GENERATED_DIR ${CMAKE_BINARY_DIR}/area512_seed_generated)
set(AREA512_SEED_STAGING_DIR ${CMAKE_BINARY_DIR}/area512_seed_staging)
set(AREA512_SEED_TAR ${CMAKE_BINARY_DIR}/area512_seed.tar)
file(GLOB AREA512_SEED_TOP_ENTRIES RELATIVE ${AREA512_SEED_DIR} ${AREA512_SEED_DIR}/*)
file(GLOB_RECURSE AREA512_SEED_FILES ${AREA512_SEED_DIR}/*)

if(AREA512_EXT_DISPLAY)
  set(AREA512_THEME_DEFAULT_BACKGROUND_IMAGE /share/backgrounds/emblem_tft.rgb565)
  set(AREA512_THEME_BLUE_BACKGROUND_IMAGE /share/backgrounds/space_tft.rgb565)
else()
  set(AREA512_THEME_DEFAULT_BACKGROUND_IMAGE /share/backgrounds/emblem.rgb565)
  set(AREA512_THEME_BLUE_BACKGROUND_IMAGE /share/backgrounds/space.rgb565)
endif()

set(AREA512_SEED_GENERATED_FILES "")

foreach(AREA512_SEED_THEME_FILE etc/theme etc/theme_default.bak etc/theme_blue.bak)
  configure_file(
    ${AREA512_SEED_DIR}/${AREA512_SEED_THEME_FILE}
    ${AREA512_SEED_GENERATED_DIR}/${AREA512_SEED_THEME_FILE}
    @ONLY
  )
  list(APPEND AREA512_SEED_GENERATED_FILES ${AREA512_SEED_GENERATED_DIR}/${AREA512_SEED_THEME_FILE})
endforeach()

add_custom_command(
  OUTPUT ${AREA512_SEED_TAR}
  COMMAND ${CMAKE_COMMAND} -E rm -rf ${AREA512_SEED_STAGING_DIR}
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${AREA512_SEED_DIR} ${AREA512_SEED_STAGING_DIR}
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${AREA512_SEED_GENERATED_DIR} ${AREA512_SEED_STAGING_DIR}
  COMMAND ${CMAKE_COMMAND} -E chdir ${AREA512_SEED_STAGING_DIR}
    ${CMAKE_COMMAND} -E tar cf ${AREA512_SEED_TAR} --format=gnutar -- ${AREA512_SEED_TOP_ENTRIES}
  DEPENDS ${AREA512_SEED_FILES} ${AREA512_SEED_GENERATED_FILES}
  VERBATIM
)

target_add_binary_data(${COMPONENT_LIB} ${AREA512_SEED_TAR} BINARY DEPENDS ${AREA512_SEED_TAR})
