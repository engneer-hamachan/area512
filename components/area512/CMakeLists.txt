# Area512: Cardputer (ESP32-S3) firmware, femtoruby (mrubyc) VM only.

# Pristine gem ports come from ./R2P2-ESP32; the build root stays vendored.
set(PURE_PICORUBY ${COMPONENT_DIR}/../../R2P2-ESP32/components/picoruby-esp32/picoruby)

set(MRUBY_CONFIG_STRING "${COMPONENT_DIR}/build_config/area512.rb")
set(LIBMRUBY_PATH "esp32-femtoruby")
if(AREA512_TDECK)
  set(LIBMRUBY_PATH "esp32-femtoruby-tdeck")
elseif(AREA512_CYD_2432S028)
  set(LIBMRUBY_PATH "esp32-femtoruby-cyd-2432s028")
elseif(AREA512_CYD)
  set(LIBMRUBY_PATH "esp32-femtoruby-cyd")
elseif(AREA512_EXT_DISPLAY)
  set(LIBMRUBY_PATH "esp32-femtoruby-captft")
endif()
file(MAKE_DIRECTORY ${PURE_PICORUBY}/build/${LIBMRUBY_PATH}/mrbgems)
set(CUSTOM_TARGET_NAME "picoruby")
set(ADDITIONAL_INCLUDE_DIRS
  ${PURE_PICORUBY}/build/${LIBMRUBY_PATH}/mrbgems
)
set(ADDITIONAL_DEFINITIONS
  -DMRBC_TICK_UNIT=10
  -DMRBC_TIMESLICE_TICK_COUNT=1
  -DMRBC_USE_FLOAT=2
  -DMRC_CUSTOM_ALLOC
  -DPICORB_VM_MRUBYC
)

set(AREA512_FILER_INCLUDE_DIRS ${COMPONENT_DIR}/mrbgems/picoruby-area512-filer/src)
if(AREA512_CYD)
  list(
    PREPEND
    AREA512_FILER_INCLUDE_DIRS
    ${COMPONENT_DIR}/mrbgems/picoruby-area512-filer/cyd/src
  )
elseif(AREA512_EXT_DISPLAY)
  list(
    PREPEND
    AREA512_FILER_INCLUDE_DIRS
    ${COMPONENT_DIR}/mrbgems/picoruby-area512-filer/captft/src
  )
endif()

idf_component_register(
  SRCS
    ${COMPONENT_DIR}/area512.c
    ${COMPONENT_DIR}/area512_arena.c
    ${COMPONENT_DIR}/area512_task_replacement.c
    ${COMPONENT_DIR}/area512_ruby_vm.c
    ${COMPONENT_DIR}/filer_shell.c
    ${COMPONENT_DIR}/ports/tree_sitter_port.c
    # esp32 machine port forked here for console (screen+keyboard) integration.
    ${COMPONENT_DIR}/ports/machine_port.c
    ${PURE_PICORUBY}/mrbgems/picoruby-machine/ports/common/machine.c
    ${PURE_PICORUBY}/mrbgems/picoruby-io-console/ports/esp32/io-console.c
    ${PURE_PICORUBY}/mrbgems/picoruby-env/ports/esp32/env.c
    ${PURE_PICORUBY}/mrbgems/picoruby-gpio/ports/esp32/gpio.c
    ${PURE_PICORUBY}/mrbgems/picoruby-adc/ports/esp32/adc.c
    ${PURE_PICORUBY}/mrbgems/picoruby-rng/ports/esp32/rng.c
    ${PURE_PICORUBY}/mrbgems/picoruby-watchdog/ports/esp32/watchdog.c
    ${PURE_PICORUBY}/mrbgems/picoruby-i2c/ports/esp32/i2c.c
    ${PURE_PICORUBY}/mrbgems/picoruby-spi/ports/esp32/spi.c
    ${PURE_PICORUBY}/mrbgems/picoruby-require/ports/esp32/platform.c
  INCLUDE_DIRS
    ${COMPONENT_DIR}
    ${PURE_PICORUBY}/include
    ${PURE_PICORUBY}/mrbgems/picoruby-mrubyc/lib/mrubyc/src
    ${PURE_PICORUBY}/mrbgems/picoruby-machine/include
    ${PURE_PICORUBY}/mrbgems/picoruby-io-console/include
    ${PURE_PICORUBY}/mrbgems/mruby-compiler2/include
    ${PURE_PICORUBY}/mrbgems/mruby-compiler2/lib/prism/include
    ${ADDITIONAL_INCLUDE_DIRS}
  PRIV_INCLUDE_DIRS
    ${COMPONENT_DIR}/mrbgems/micropython-ti/include
    ${COMPONENT_DIR}/mrbgems/picoruby-area512-sandbox/src
    ${COMPONENT_DIR}/mrbgems/picoruby-area512-compile/src
    ${COMPONENT_DIR}/mrbgems/picoruby-area512-edit/src
    ${COMPONENT_DIR}/mrbgems/picoruby-area512-dot/src
    ${COMPONENT_DIR}/mrbgems/picoruby-area512-repl/src
    ${AREA512_FILER_INCLUDE_DIRS}
    ${PURE_PICORUBY}/mrbgems/picoruby-watchdog/include
  PRIV_REQUIRES
    esp_driver_gpio
    esp_driver_i2c
    esp_adc
    esp_timer
    esp_hw_support
    spi_flash
    nvs_flash
    esp_driver_usb_serial_jtag
    esp_driver_uart
    area512_hal
    area512_micropython
    area512_tree_sitter
)

# Status line messages are cut to MESSAGE_MAX on purpose, like the Ruby
# filer's copy_string did.
set_source_files_properties(
  ${COMPONENT_DIR}/filer_shell.c
  PROPERTIES COMPILE_OPTIONS "-Wno-format-truncation"
)

if(AREA512_CYD_2432S028)
  set_source_files_properties(
    ${COMPONENT_DIR}/area512.c
    ${COMPONENT_DIR}/area512_arena.c
    ${COMPONENT_DIR}/filer_shell.c
    PROPERTIES COMPILE_DEFINITIONS "AREA512_CYD_2432S028"
  )
endif()

add_definitions(
  ${ADDITIONAL_DEFINITIONS}
  -DMRBC_CONVERT_CRLF=1
  -DESP32_PLATFORM
  -DNDEBUG
)

target_include_directories(${COMPONENT_LIB} BEFORE PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

set(PICORUBY_DIR ${PURE_PICORUBY})
set(LIBMRUBY_FILE ${PICORUBY_DIR}/build/${LIBMRUBY_PATH}/lib/libmruby.a)

# Expose sdkconfig values to build config via env vars
set(MRUBY_ENV
  MRUBY_CONFIG=${MRUBY_CONFIG_STRING}
  CONFIG_IDF_TARGET=${CONFIG_IDF_TARGET}
  AREA512_EXT_DISPLAY=${AREA512_EXT_DISPLAY}
  AREA512_TDECK=${AREA512_TDECK}
  AREA512_CYD=${AREA512_CYD}
  AREA512_CYD_2432S028=${AREA512_CYD_2432S028}
)

# Build PicoRuby
file(GLOB_RECURSE MRUBY_SOURCE_FILES CONFIGURE_DEPENDS
  ${COMPONENT_DIR}/mrbgems/*.c
  ${COMPONENT_DIR}/mrbgems/*.h
  ${COMPONENT_DIR}/mrbgems/*.rb
  ${COMPONENT_DIR}/mrbgems/*.rake
)

if(AREA512_CYD)
  set(AREA512_CYD_TI_GENERATED_DIR ${CMAKE_SOURCE_DIR}/build/cyd/ti_generated)
  list(APPEND MRUBY_ENV AREA512_CYD_TI_GENERATED_DIR=${AREA512_CYD_TI_GENERATED_DIR})
  file(GLOB_RECURSE AREA512_CYD_TI_GENERATED_FILES CONFIGURE_DEPENDS
    ${AREA512_CYD_TI_GENERATED_DIR}/*.c
    ${AREA512_CYD_TI_GENERATED_DIR}/*.h
  )
  list(APPEND MRUBY_SOURCE_FILES ${AREA512_CYD_TI_GENERATED_FILES})
endif()
add_custom_command(
  OUTPUT ${LIBMRUBY_FILE}
  COMMAND ${CMAKE_COMMAND} -E echo ${MRUBY_ENV} rake
  COMMAND ${CMAKE_COMMAND} -E env ${MRUBY_ENV} rake
  WORKING_DIRECTORY ${PICORUBY_DIR}
  DEPENDS ${MRUBY_CONFIG_STRING}
    ${MRUBY_SOURCE_FILES}
    ${COMPONENT_DIR}/../area512_hal/include/area512_hal.h
    ${COMPONENT_DIR}/../area512_hal/captft/include/area512_hal.h
    ${COMPONENT_DIR}/../area512_hal/cyd/include/area512_hal.h
  COMMENT "PicoRuby build"
  VERBATIM
)

add_prebuilt_library(
  libmruby ${LIBMRUBY_FILE}
  REQUIRES ${COMPONENT_NAME}
)
target_link_libraries(${COMPONENT_LIB} PRIVATE libmruby)
target_include_directories(
  ${COMPONENT_LIB}
  PRIVATE
    ${PURE_PICORUBY}/mrbgems/mruby-compiler2/include
    ${PURE_PICORUBY}/mrbgems/mruby-compiler2/lib/prism/include
    ${ADDITIONAL_INCLUDE_DIRS}
)

add_custom_target(
  ${CUSTOM_TARGET_NAME} DEPENDS ${LIBMRUBY_FILE}
)
add_dependencies(${COMPONENT_LIB} ${CUSTOM_TARGET_NAME})

# Compile Ruby files
set(RUBY_FILES environment_task irb_task)
set(PICORBC ${PURE_PICORUBY}/bin/picorbc)
set(GENERATED_C_FILES "")

foreach(rb ${RUBY_FILES})
  set(C_FILE ${CMAKE_CURRENT_BINARY_DIR}/mrb/${rb}.c)
  set(RB_FILE ${COMPONENT_DIR}/mrblib/${rb}.rb)
  add_custom_command(
    OUTPUT ${C_FILE}
    COMMAND ${CMAKE_COMMAND} -E echo "Compiling ${RB_FILE}..."
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/mrb
    COMMAND ${PICORBC} -B${rb} -o${C_FILE} ${RB_FILE}
    DEPENDS ${CUSTOM_TARGET_NAME} ${RB_FILE}
    COMMENT "Compiling ${RB_FILE}"
    VERBATIM
  )
  list(APPEND GENERATED_C_FILES ${C_FILE})
endforeach(rb)

target_sources(${COMPONENT_LIB} PRIVATE ${GENERATED_C_FILES})
