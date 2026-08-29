set(MICROPYTHON_SOURCE_DIRECTORY ${COMPONENT_DIR}/../micropython)
set(MICROPYTHON_EMBED_DIRECTORY ${COMPONENT_DIR}/micropython_embed)
set(
  MICROPYTHON_GENERATED_VERSION_HEADER
  ${MICROPYTHON_EMBED_DIRECTORY}/genhdr/mpversion.h
)

file(
  GLOB
  MICROPYTHON_CORE_SOURCE_PATHS
  ${MICROPYTHON_SOURCE_DIRECTORY}/py/*.c
)

set(MICROPYTHON_EMBED_SOURCE_PATHS "")

foreach(MICROPYTHON_CORE_SOURCE_PATH ${MICROPYTHON_CORE_SOURCE_PATHS})
  get_filename_component(
    MICROPYTHON_CORE_SOURCE_FILE_NAME
    ${MICROPYTHON_CORE_SOURCE_PATH}
    NAME
  )

  list(
    APPEND
    MICROPYTHON_EMBED_SOURCE_PATHS
    ${MICROPYTHON_EMBED_DIRECTORY}/py/${MICROPYTHON_CORE_SOURCE_FILE_NAME}
  )
endforeach()

list(
  APPEND
  MICROPYTHON_EMBED_SOURCE_PATHS
  ${MICROPYTHON_EMBED_DIRECTORY}/shared/runtime/gchelper_generic.c
  ${MICROPYTHON_EMBED_DIRECTORY}/port/embed_util.c
)

file(
  MAKE_DIRECTORY
  ${MICROPYTHON_EMBED_DIRECTORY}
  ${MICROPYTHON_EMBED_DIRECTORY}/port
)

idf_component_register(
  SRCS
    area512_micropython_python_file.c
    mphalport.c
    modules/area512_micropython_sprite.c
    modules/area512_micropython_dot.c
    modules/area512_micropython_console.c
    modules/area512_micropython_io.c
    modules/area512_micropython_sd.c
    modules/area512_micropython_gpio.c
    modules/area512_micropython_adc.c
    modules/area512_micropython_widget.c
    modules/area512_micropython_sleep.c
    modules/area512_micropython_rng.c
    ${MICROPYTHON_EMBED_SOURCE_PATHS}
  INCLUDE_DIRS
    .
    include
    ${MICROPYTHON_EMBED_DIRECTORY}
    ${MICROPYTHON_EMBED_DIRECTORY}/port
  REQUIRES
    area512_hal
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE NDEBUG)

# Kept private: gpio.h and io-console.h have generic names that would collide
# with other components if these directories were exported. The widget toolkit
# and the dot image core are VM-independent C that live in picoruby gems; the
# gems compile them and this component only calls into them.
target_include_directories(
  ${COMPONENT_LIB}
  PRIVATE
    ${COMPONENT_DIR}/../../R2P2-ESP32/components/picoruby-esp32/picoruby/mrbgems/picoruby-gpio/include
    ${COMPONENT_DIR}/../../R2P2-ESP32/components/picoruby-esp32/picoruby/mrbgems/picoruby-io-console/include
    ${COMPONENT_DIR}/../area512/mrbgems/picoruby-area512-widget/src
    ${COMPONENT_DIR}/../area512/mrbgems/picoruby-area512-dot/src
)

set_source_files_properties(
  ${MICROPYTHON_EMBED_SOURCE_PATHS}
  PROPERTIES GENERATED TRUE
)

add_custom_command(
  OUTPUT ${MICROPYTHON_GENERATED_VERSION_HEADER}
  COMMAND
    make
    -f
    ${COMPONENT_DIR}/micropython_embed.mk
  WORKING_DIRECTORY ${COMPONENT_DIR}
  DEPENDS
    ${COMPONENT_DIR}/micropython_embed.mk
    ${COMPONENT_DIR}/mpconfigport.h
    ${COMPONENT_DIR}/modules/area512_micropython_sprite.c
    ${COMPONENT_DIR}/modules/area512_micropython_dot.c
    ${COMPONENT_DIR}/modules/area512_micropython_console.c
    ${COMPONENT_DIR}/modules/area512_micropython_io.c
    ${COMPONENT_DIR}/modules/area512_micropython_sd.c
    ${COMPONENT_DIR}/modules/area512_micropython_gpio.c
    ${COMPONENT_DIR}/modules/area512_micropython_adc.c
    ${COMPONENT_DIR}/modules/area512_micropython_widget.c
    ${COMPONENT_DIR}/modules/area512_micropython_sleep.c
    ${COMPONENT_DIR}/modules/area512_micropython_rng.c
  COMMENT "Generate MicroPython embed sources"
  VERBATIM
)

add_custom_target(
  area512_micropython_embed
  DEPENDS ${MICROPYTHON_GENERATED_VERSION_HEADER}
)

add_dependencies(${COMPONENT_LIB} area512_micropython_embed)
