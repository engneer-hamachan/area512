# Area512: Cardputer (ESP32-S3) firmware, femtoruby (mrubyc) VM only.

# Pristine gem ports come from ./R2P2-ESP32; the build root stays vendored.
set(PURE_PICORUBY ${COMPONENT_DIR}/../../R2P2-ESP32/components/picoruby-esp32/picoruby)

file(MAKE_DIRECTORY ${PURE_PICORUBY}/build/esp32-femtoruby/mrbgems)
set(MRUBY_CONFIG_STRING "${COMPONENT_DIR}/build_config/area512.rb")
set(LIBMRUBY_PATH "esp32-femtoruby")
set(CUSTOM_TARGET_NAME "picoruby")
set(ADDITIONAL_INCLUDE_DIRS
  ${PURE_PICORUBY}/build/esp32-femtoruby/mrbgems
)
set(ADDITIONAL_DEFINITIONS
  -DMRBC_TICK_UNIT=10
  -DMRBC_TIMESLICE_TICK_COUNT=1
  -DMRBC_USE_FLOAT=2
  -DMRC_CUSTOM_ALLOC
  -DPICORB_VM_MRUBYC
)

if(DEFINED ENV{HEAP_SIZE})
  list(APPEND ADDITIONAL_DEFINITIONS -DHEAP_SIZE=$ENV{HEAP_SIZE})
endif()

idf_component_register(
  SRCS
    ${COMPONENT_DIR}/area512.c
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
  PRIV_REQUIRES
    esp_driver_gpio
    esp_driver_i2c
    esp_adc
    esp_timer
    esp_hw_support
    spi_flash
    nvs_flash
    esp_driver_usb_serial_jtag
    area512_hal
)

add_definitions(
  ${ADDITIONAL_DEFINITIONS}
  -DMRBC_CONVERT_CRLF=1
  -DESP32_PLATFORM
  -DNDEBUG
)

set(PICORUBY_DIR ${PURE_PICORUBY})
set(LIBMRUBY_FILE ${PICORUBY_DIR}/build/${LIBMRUBY_PATH}/lib/libmruby.a)

# Expose sdkconfig values to build config via env vars
set(MRUBY_ENV
  MRUBY_CONFIG=${MRUBY_CONFIG_STRING}
  CONFIG_IDF_TARGET=${CONFIG_IDF_TARGET}
)

# Build PicoRuby
add_custom_command(
  OUTPUT ${LIBMRUBY_FILE}
  COMMAND ${CMAKE_COMMAND} -E echo ${MRUBY_ENV} rake
  COMMAND ${CMAKE_COMMAND} -E env ${MRUBY_ENV} rake
  WORKING_DIRECTORY ${PICORUBY_DIR}
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
set(RUBY_FILES main_task)
set(PICORBC ${PURE_PICORUBY}/bin/picorbc)
set(GENERATED_C_FILES "")

foreach(rb ${RUBY_FILES})
  set(C_FILE ${COMPONENT_DIR}/mrb/${rb}.c)
  set(RB_FILE ${COMPONENT_DIR}/mrblib/${rb}.rb)
  add_custom_command(
    OUTPUT ${C_FILE}
    COMMAND ${CMAKE_COMMAND} -E echo "Compiling ${RB_FILE}..."
    COMMAND ${CMAKE_COMMAND} -E make_directory ${COMPONENT_DIR}/mrb
    COMMAND ${PICORBC} -B${rb} -o${C_FILE} ${RB_FILE}
    DEPENDS ${CUSTOM_TARGET_NAME} ${RB_FILE}
    COMMENT "Compiling ${RB_FILE}"
    VERBATIM
  )
  list(APPEND GENERATED_C_FILES ${C_FILE})
endforeach(rb)

target_sources(${COMPONENT_LIB} PRIVATE ${GENERATED_C_FILES})
