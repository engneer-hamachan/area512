#include "touch_setting.h"

#include "area512_hal.h"

#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TOUCH_SETTING_PATH "etc/touch"
#define TOUCH_SETTING_LINE_BYTE_SIZE 64
#define CALIBRATION_KEY "calibration"
#define DRAG_START_DISTANCE_KEY "drag_start_distance"
#define DRAG_AXIS_DECISION_DISTANCE_KEY "drag_axis_decision_distance"
#define SWIPE_DISTANCE_KEY "swipe_distance"

static bool s_has_calibration = false;
static uint16_t s_calibration_values[AREA512_TOUCH_CALIBRATION_VALUE_COUNT];

void
area512_read_current_touch_distances(TouchSetting *touch_setting) {
  touch_setting->drag_start_distance = area512_read_touch_drag_start_distance();

  touch_setting->drag_axis_decision_distance =
    area512_read_touch_drag_axis_decision_distance();

  touch_setting->swipe_distance = area512_read_touch_swipe_distance();
}

void
area512_read_current_touch_setting(TouchSetting *touch_setting) {
  touch_setting->has_calibration = s_has_calibration;

  memcpy(
    touch_setting->calibration_values,
    s_calibration_values,
    sizeof(touch_setting->calibration_values)
  );

  area512_read_current_touch_distances(touch_setting);
}

void
area512_apply_touch_setting(const TouchSetting *touch_setting) {
  if (touch_setting->has_calibration)
    area512_display_set_touch_calibration(touch_setting->calibration_values);
  else
    area512_display_reset_touch_calibration();

  s_has_calibration = touch_setting->has_calibration;

  memcpy(
    s_calibration_values,
    touch_setting->calibration_values,
    sizeof(s_calibration_values)
  );

  area512_set_touch_drag_start_distance(touch_setting->drag_start_distance);

  area512_set_touch_drag_axis_decision_distance(
    touch_setting->drag_axis_decision_distance
  );

  area512_set_touch_swipe_distance(touch_setting->swipe_distance);
}

static bool
parse_calibration_values(
  const char *value_text,
  uint16_t calibration_values[AREA512_TOUCH_CALIBRATION_VALUE_COUNT]
) {

  const char *cursor = value_text;

  for (int i = 0; i < AREA512_TOUCH_CALIBRATION_VALUE_COUNT; i++) {
    if (i > 0) {
      if (*cursor != ',')
        return false;

      cursor++;
    }

    char *calibration_value_end;
    long calibration_value =
      strtol(cursor, &calibration_value_end, 10);

    if (
      calibration_value_end == cursor ||
      calibration_value < 0 ||
      calibration_value > UINT16_MAX
    )
      return false;

    calibration_values[i] = (uint16_t)calibration_value;
    cursor = calibration_value_end;
  }

  return *cursor == '\0';
}

static void
parse_distance_value(const char *value_text, int *distance_value) {
  char *parsed_distance_value_end;

  long parsed_distance_value =
    strtol(value_text, &parsed_distance_value_end, 10);

  if (
    parsed_distance_value_end == value_text ||
    *parsed_distance_value_end != '\0' ||
    parsed_distance_value <= 0 ||
    parsed_distance_value > INT_MAX
  )
    return;

  *distance_value = (int)parsed_distance_value;
}

static void
split_line_and_assign_touch_setting(char *line, TouchSetting *touch_setting) {
  char *equal_sign = strchr(line, '=');

  if (equal_sign == NULL)
    return;

  *equal_sign = '\0';

  const char *line_key = line;
  const char *value_text = equal_sign + 1;

  if (strcmp(line_key, CALIBRATION_KEY) == 0) {
    uint16_t calibration_values[AREA512_TOUCH_CALIBRATION_VALUE_COUNT];

    if (!parse_calibration_values(value_text, calibration_values))
      return;

    touch_setting->has_calibration = true;

    memcpy(
      touch_setting->calibration_values,
      calibration_values,
      sizeof(calibration_values)
    );

    return;
  }

  if (strcmp(line_key, DRAG_START_DISTANCE_KEY) == 0)
    parse_distance_value(
      value_text,
      &touch_setting->drag_start_distance
    );

  else if (strcmp(line_key, DRAG_AXIS_DECISION_DISTANCE_KEY) == 0)
    parse_distance_value(
      value_text,
      &touch_setting->drag_axis_decision_distance
    );

  else if (strcmp(line_key, SWIPE_DISTANCE_KEY) == 0)
    parse_distance_value(value_text, &touch_setting->swipe_distance);
}

static void
remove_line_ending(char *line) {
  line[strcspn(line, "\r\n")] = '\0';
}

static FILE *
open_touch_setting_file(const char *mode) {
  char resolved_path[AREA512_PATH_MAX];

  if (
    area512_resolve_data_path(
      TOUCH_SETTING_PATH,
      resolved_path,
      sizeof(resolved_path)
    ) < 0
  )
    return NULL;

  return fopen(resolved_path, mode);
}

void
area512_load_and_apply_touch_setting(void) {
  FILE *file = open_touch_setting_file("r");

  if (file == NULL)
    return;

  TouchSetting touch_setting;
  char line[TOUCH_SETTING_LINE_BYTE_SIZE];

  area512_read_current_touch_setting(&touch_setting);

  while (fgets(line, sizeof(line), file) != NULL) {
    remove_line_ending(line);
    split_line_and_assign_touch_setting(line, &touch_setting);
  }

  fclose(file);

  area512_apply_touch_setting(&touch_setting);
}

bool
area512_save_touch_setting(const TouchSetting *touch_setting) {
  FILE *file = open_touch_setting_file("w");

  if (file == NULL)
    return false;

  if (touch_setting->has_calibration) {
    fprintf(file, CALIBRATION_KEY "=");

    for (int i = 0; i < AREA512_TOUCH_CALIBRATION_VALUE_COUNT; i++)
      fprintf(
        file,
        i == 0 ? "%" PRIu16 : ",%" PRIu16,
        touch_setting->calibration_values[i]
      );

    fprintf(file, "\n");
  }

  fprintf(
    file,
    DRAG_START_DISTANCE_KEY "=%d\n",
    touch_setting->drag_start_distance
  );

  fprintf(
    file,
    DRAG_AXIS_DECISION_DISTANCE_KEY "=%d\n",
    touch_setting->drag_axis_decision_distance
  );

  fprintf(file, SWIPE_DISTANCE_KEY "=%d\n", touch_setting->swipe_distance);

  bool has_write_error = ferror(file) != 0;

  return fclose(file) == 0 && !has_write_error;
}
