#pragma once

#include "area512_display.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  bool has_calibration;
  uint16_t calibration_values[AREA512_TOUCH_CALIBRATION_VALUE_COUNT];
  int drag_start_distance;
  int drag_axis_decision_distance;
  int swipe_distance;
} TouchSetting;

void area512_read_current_touch_distances(TouchSetting *touch_setting);
void area512_read_current_touch_setting(TouchSetting *touch_setting);
void area512_apply_touch_setting(const TouchSetting *touch_setting);
bool area512_save_touch_setting(const TouchSetting *touch_setting);
