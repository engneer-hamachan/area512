#pragma once

#include "area512_display.h"

#include <stdbool.h>
#include <stdint.h>

#define TOUCH_CALIBRATION_POINT_COUNT 4

typedef struct {
  int panel_x;
  int panel_y;
  int raw_x;
  int raw_y;
} TouchCalibrationPoint;

bool area512_compute_touch_calibration_values(
  const TouchCalibrationPoint calibration_points[TOUCH_CALIBRATION_POINT_COUNT],
  int panel_width,
  int panel_height,
  uint16_t calibration_values[AREA512_TOUCH_CALIBRATION_VALUE_COUNT]
);
