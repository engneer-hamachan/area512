set(AREA512_DISPLAY_DIR "")
if(AREA512_EXT_DISPLAY)
  set(AREA512_DISPLAY_DIR "captft/")
endif()

set(AREA512_KEYBOARD_SOURCE console/keyboard.cpp)
if(AREA512_TDECK)
  set(AREA512_KEYBOARD_SOURCE console/tdeck_keyboard.cpp)
endif()

idf_component_register(
  SRCS
    console/input.cpp
    ${AREA512_KEYBOARD_SOURCE}
    console/getch.cpp
    ${AREA512_DISPLAY_DIR}console/output.c
    console/system_metrics.cpp
    ${AREA512_DISPLAY_DIR}gfx/area512_display.cpp
    ${AREA512_DISPLAY_DIR}gfx/area512_gfx.cpp
    gfx/area512_crt.cpp
    sd/area512_data_path.c
    sd/area512_manifest.c
    sd/area512_sd.c
    sd/area512_seed.c
    theme/area512_theme.c
    ui/area512_default_ui.c
    ui/area512_next_app.c
    usb/area512_usb_receive.c
    ota/area512_app_image.c
    animation/area512_animation_timer.c
    animation/filer_cursor_animation.c
    animation/filer_terminal_cursor_animation.c
    animation/filer_window_box.c
    animation/filer_window_open_animation.c
    animation/filer_window_close_animation.c
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
    app_update
    bootloader_support
    esp_partition
    esp_timer
)

# area512 requires this component, so its directory is a private include
# path rather than a requirement.
target_include_directories(${COMPONENT_LIB} PRIVATE ${COMPONENT_DIR}/../area512)

if(AREA512_EXT_DISPLAY)
  target_compile_definitions(${COMPONENT_LIB} PUBLIC AREA512_EXT_DISPLAY)

  if(NOT AREA512_TDECK)
    target_sources(${COMPONENT_LIB} PRIVATE
      captft/gfx/area512_internal_display.cpp
    )
    target_compile_definitions(${COMPONENT_LIB} PUBLIC AREA512_INTERNAL_DISPLAY)
  endif()

  if(AREA512_CAPTFT_ILI9341)
    set_source_files_properties(
      captft/gfx/area512_display.cpp
      PROPERTIES COMPILE_DEFINITIONS "AREA512_CAPTFT_ILI9341"
    )
  endif()
endif()

if(AREA512_TDECK)
  target_compile_definitions(${COMPONENT_LIB} PRIVATE AREA512_TDECK)
endif()

if(AREA512_CARDPUTER_V11)
  set_source_files_properties(
    console/keyboard.cpp
    PROPERTIES COMPILE_DEFINITIONS "AREA512_CARDPUTER_V11"
  )
endif()

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
