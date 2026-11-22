#pragma once

#if defined(AREA512_CYD)
#include <stdbool.h>
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

void area512_display_init(void);
void area512_display_draw_boot_splash(void);
void *area512_display_device(void);

#if defined(AREA512_CYD)
// LovyanGFX setTouchCalibrate takes a raw x and y for each of four corners.
#define AREA512_TOUCH_CALIBRATION_VALUE_COUNT 8

bool area512_display_read_raw_touch_point(int *raw_x, int *raw_y);
void area512_display_convert_screen_to_touch_calibration_point(
  int screen_x,
  int screen_y,
  int *calibration_x,
  int *calibration_y
);
void area512_display_read_touch_calibration_size(
  int *calibration_width,
  int *calibration_height
);
void area512_display_set_touch_calibration(
  const uint16_t calibration_values[AREA512_TOUCH_CALIBRATION_VALUE_COUNT]
);
void area512_display_reset_touch_calibration(void);
#endif

#ifdef __cplusplus
}
#endif
