#pragma once

#include <port/mpconfigport_common.h>

#define MICROPY_CONFIG_ROM_LEVEL (MICROPY_CONFIG_ROM_LEVEL_MINIMUM)
#define MICROPY_ENABLE_COMPILER (1)
#define MICROPY_ENABLE_FINALISER (1)
#define MICROPY_ENABLE_GC (1)
#define MICROPY_GCREGS_SETJMP (1)
#define MICROPY_GC_SPLIT_HEAP (1)
#define MICROPY_GC_SPLIT_HEAP_AUTO (1)
#define MICROPY_NLR_SETJMP (1)
#define MICROPY_PY_GC (1)
#define MICROPY_PY_SYS (0)

#define MICROPY_PERSISTENT_CODE_LOAD (1)
#define MICROPY_PERSISTENT_CODE_SAVE (1)

// ADC.read_voltage() returns ADC_read_voltage()'s picorb_float_t, which
// MRBC_USE_FLOAT=2 makes double; complex would come along for free otherwise.
#define MICROPY_FLOAT_IMPL (MICROPY_FLOAT_IMPL_DOUBLE)
#define MICROPY_PY_BUILTINS_COMPLEX (0)

// Declared incomplete: this header is read before py/obj.h defines the type.
extern const struct _mp_obj_type_t area512_micropython_sprite_type;
extern const struct _mp_obj_type_t area512_micropython_display_type;
extern const struct _mp_obj_type_t area512_micropython_dot_type;
extern const struct _mp_obj_type_t area512_micropython_console_type;
extern const struct _mp_obj_type_t area512_micropython_io_type;
extern const struct _mp_obj_base_t area512_micropython_stdin_instance;
extern const struct _mp_obj_type_t area512_micropython_sd_type;
extern const struct _mp_obj_type_t area512_micropython_file_type;
extern const struct _mp_obj_type_t area512_micropython_directory_type;
extern const struct _mp_obj_type_t area512_micropython_gpio_type;
extern const struct _mp_obj_type_t area512_micropython_adc_type;
extern const struct _mp_obj_type_t area512_micropython_widget_type;
extern const struct _mp_obj_type_t area512_micropython_widget_list_type;
extern const struct _mp_obj_type_t area512_micropython_widget_text_view_type;
extern const struct _mp_obj_type_t area512_micropython_rng_type;
extern const struct _mp_obj_fun_builtin_fixed_t
  area512_micropython_sleep_seconds_callable;
extern const struct _mp_obj_fun_builtin_fixed_t
  area512_micropython_sleep_milliseconds_callable;

#define MICROPY_PORT_BUILTINS                                                  \
  {MP_ROM_QSTR(MP_QSTR_Sprite), MP_ROM_PTR(&area512_micropython_sprite_type)}, \
    {MP_ROM_QSTR(MP_QSTR_Display),                                             \
     MP_ROM_PTR(&area512_micropython_display_type)},                           \
    {MP_ROM_QSTR(MP_QSTR_Dot), MP_ROM_PTR(&area512_micropython_dot_type)},     \
    {MP_ROM_QSTR(MP_QSTR_Console),                                             \
     MP_ROM_PTR(&area512_micropython_console_type)},                           \
    {MP_ROM_QSTR(MP_QSTR_IO), MP_ROM_PTR(&area512_micropython_io_type)},       \
    {MP_ROM_QSTR(MP_QSTR_STDIN),                                               \
     MP_ROM_PTR(&area512_micropython_stdin_instance)},                         \
    {MP_ROM_QSTR(MP_QSTR_SD), MP_ROM_PTR(&area512_micropython_sd_type)},       \
    {MP_ROM_QSTR(MP_QSTR_File), MP_ROM_PTR(&area512_micropython_file_type)},   \
    {MP_ROM_QSTR(MP_QSTR_Dir), MP_ROM_PTR(&area512_micropython_directory_type) \
    },                                                                         \
    {MP_ROM_QSTR(MP_QSTR_GPIO), MP_ROM_PTR(&area512_micropython_gpio_type)},   \
    {MP_ROM_QSTR(MP_QSTR_ADC), MP_ROM_PTR(&area512_micropython_adc_type)},     \
    {MP_ROM_QSTR(MP_QSTR_Widget), MP_ROM_PTR(&area512_micropython_widget_type) \
    },                                                                         \
    {MP_ROM_QSTR(MP_QSTR_WidgetList),                                          \
     MP_ROM_PTR(&area512_micropython_widget_list_type)},                       \
    {MP_ROM_QSTR(MP_QSTR_WidgetTextView),                                      \
     MP_ROM_PTR(&area512_micropython_widget_text_view_type)},                  \
    {MP_ROM_QSTR(MP_QSTR_RNG), MP_ROM_PTR(&area512_micropython_rng_type)},     \
    {MP_ROM_QSTR(MP_QSTR_sleep),                                               \
     MP_ROM_PTR(&area512_micropython_sleep_seconds_callable)},                 \
    {MP_ROM_QSTR(MP_QSTR_sleep_ms),                                            \
     MP_ROM_PTR(&area512_micropython_sleep_milliseconds_callable)},
