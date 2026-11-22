#include "touch_calibration.h"
#include "touch_setting.h"

#include "area512_hal.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdio.h>

#define BAND_HEIGHT 32
#define BAND_FONT_SIZE 12
#define TEXT_LEFT_ON_BAND 16
#define TEXT_BYTE_SIZE 48

#define POLL_INTERVAL_MILLISECONDS 10
#define MESSAGE_HOLD_MILLISECONDS 2000

#define CALIBRATION_CROSS_INSET 20
#define CALIBRATION_CROSS_ARM_LENGTH 10
#define CALIBRATION_CROSS_CIRCLE_RADIUS 4
#define CALIBRATION_MINIMUM_SAMPLE_COUNT 10
#define CALIBRATION_MAXIMUM_SAMPLE_COUNT 64
#define CALIBRATION_TIMEOUT_MILLISECONDS 20000

#define DISTANCE_STEP 2
#define MINIMUM_DISTANCE 2

#define DISTANCE_TITLE_BAND_INDEX 0
#define CALIBRATION_TITLE_BAND_INDEX 2
#define CALIBRATION_HINT_BAND_INDEX 3
#define CALIBRATION_STOPPED_MESSAGE_BAND_INDEX 3
#define TEST_AREA_TOP_BAND_INDEX 4
#define CANCEL_AND_SAVE_BUTTON_BAND_INDEX 6
#define SAVE_ERROR_MESSAGE_BAND_INDEX 6

#define BAND_PADDING 4
#define BAND_INNER_HEIGHT (BAND_HEIGHT - BAND_PADDING * 2)
#define BAND_INNER_SCREEN_TOP(band_index) \
  ((band_index) * BAND_HEIGHT + BAND_PADDING)

typedef struct {
  int x;
  int y;
} ScreenPoint;

typedef struct {
  int left;
  int top;
  int width;
  int height;
} ScreenRectangle;

typedef struct {
  ScreenRectangle rectangle;
  const char *label;
} ScreenButton;

typedef struct {
  int band_index;
  const char *label;
  int (*read_distance)(void);
  void (*set_distance)(int distance);
} DistanceSetting;

static const ScreenButton CANCEL_BUTTON = {
  {
    16,
    BAND_INNER_SCREEN_TOP(CANCEL_AND_SAVE_BUTTON_BAND_INDEX),
    136,
    BAND_INNER_HEIGHT
  },
  "CANCEL"
};

static const ScreenButton SAVE_BUTTON = {
  {
    168,
    BAND_INNER_SCREEN_TOP(CANCEL_AND_SAVE_BUTTON_BAND_INDEX),
    136,
    BAND_INNER_HEIGHT
  },
  "SAVE"
};

#define DISTANCE_DECREASE_BUTTON_SCREEN_LEFT 208
#define DISTANCE_INCREASE_BUTTON_SCREEN_LEFT 260
#define DISTANCE_BUTTON_WIDTH 44

static const DistanceSetting DISTANCE_SETTINGS[] = {
  {
    1,
    "Drag start",
    area512_read_touch_drag_start_distance,
    area512_set_touch_drag_start_distance
  },
  {
    2,
    "Drag axis",
    area512_read_touch_drag_axis_decision_distance,
    area512_set_touch_drag_axis_decision_distance
  },
  {
    3,
    "Swipe",
    area512_read_touch_swipe_distance,
    area512_set_touch_swipe_distance
  },
};

#define DISTANCE_SETTING_COUNT \
  ((int)(sizeof(DISTANCE_SETTINGS) / sizeof(DISTANCE_SETTINGS[0])))

static const ScreenRectangle TEST_AREA_RECTANGLE = {
  16,
  BAND_INNER_SCREEN_TOP(TEST_AREA_TOP_BAND_INDEX),
  288,
  BAND_INNER_HEIGHT + BAND_HEIGHT
};

static void *s_band_sprite = NULL;

static void
clear_band_sprite(void) {
  area512_sprite_fill(
    s_band_sprite,
    AREA512_QUATERNARY_SPRITE_BACKGROUND_COLOR_INDEX
  );
}

static void
push_band_sprite(int band_index) {
  area512_sprite_push(s_band_sprite, 0, band_index * BAND_HEIGHT);
}

static void
draw_text_on_band_sprite(
  int text_left_on_band,
  const char *text,
  uint32_t color_index
) {

  int text_top_on_band =
    (BAND_HEIGHT - area512_sprite_font_height(BAND_FONT_SIZE)) / 2;

  area512_sprite_text(
    s_band_sprite,
    text_left_on_band,
    text_top_on_band,
    text,
    color_index
  );
}

static void
draw_rectangle_on_band_sprite(
  int band_index,
  const ScreenRectangle *rectangle
) {

  area512_sprite_rect(
    s_band_sprite,
    rectangle->left,
    rectangle->top - band_index * BAND_HEIGHT,
    rectangle->width,
    rectangle->height,
    AREA512_QUATERNARY_SPRITE_BORDER_COLOR_INDEX
  );
}

static void
draw_button_on_band_sprite(int band_index, const ScreenButton *button) {
  int label_width =
    area512_sprite_text_width(s_band_sprite, button->label);

  draw_rectangle_on_band_sprite(band_index, &button->rectangle);

  draw_text_on_band_sprite(
    button->rectangle.left + (button->rectangle.width - label_width) / 2,
    button->label,
    AREA512_QUATERNARY_SPRITE_TEXT_COLOR_INDEX
  );
}

static void
draw_text_band(int band_index, const char *text, uint32_t color_index) {
  clear_band_sprite();
  draw_text_on_band_sprite(TEXT_LEFT_ON_BAND, text, color_index);
  push_band_sprite(band_index);
}

static int
compute_band_count(void) {
  return (area512_gfx_height() + BAND_HEIGHT - 1) / BAND_HEIGHT;
}

static void
clear_screen(void) {
  int band_count = compute_band_count();

  clear_band_sprite();

  for (int band_index = 0; band_index < band_count; band_index++)
    push_band_sprite(band_index);
}

static bool
is_touch_point_in_rectangle(
  const ScreenRectangle *rectangle,
  int touch_screen_x,
  int touch_screen_y
) {

  return touch_screen_x >= rectangle->left &&
         touch_screen_x < rectangle->left + rectangle->width &&
         touch_screen_y >= rectangle->top &&
         touch_screen_y < rectangle->top + rectangle->height;
}

static void
draw_cross_on_band_sprite(int cross_x_on_band, int cross_y_on_band) {
  area512_sprite_line(
    s_band_sprite,
    cross_x_on_band - CALIBRATION_CROSS_ARM_LENGTH,
    cross_y_on_band,
    cross_x_on_band + CALIBRATION_CROSS_ARM_LENGTH,
    cross_y_on_band,
    AREA512_QUATERNARY_SPRITE_EMPHASIS_COLOR_INDEX
  );
  area512_sprite_line(
    s_band_sprite,
    cross_x_on_band,
    cross_y_on_band - CALIBRATION_CROSS_ARM_LENGTH,
    cross_x_on_band,
    cross_y_on_band + CALIBRATION_CROSS_ARM_LENGTH,
    AREA512_QUATERNARY_SPRITE_EMPHASIS_COLOR_INDEX
  );
  area512_sprite_circle(
    s_band_sprite,
    cross_x_on_band,
    cross_y_on_band,
    CALIBRATION_CROSS_CIRCLE_RADIUS,
    AREA512_QUATERNARY_SPRITE_EMPHASIS_COLOR_INDEX
  );
}

static void
draw_calibration_screen(int cross_index, ScreenPoint cross) {
  char title_text[TEXT_BYTE_SIZE];
  int band_count = compute_band_count();

  snprintf(
    title_text,
    sizeof(title_text),
    "TOUCH SETTING 1/2  POSITION %d/%d",
    cross_index + 1,
    TOUCH_CALIBRATION_POINT_COUNT
  );

  for (int band_index = 0; band_index < band_count; band_index++) {
    clear_band_sprite();

    switch (band_index) {
    case CALIBRATION_TITLE_BAND_INDEX:
      draw_text_on_band_sprite(
        TEXT_LEFT_ON_BAND,
        title_text,
        AREA512_QUATERNARY_SPRITE_EMPHASIS_COLOR_INDEX
      );
      break;

    case CALIBRATION_HINT_BAND_INDEX:
      draw_text_on_band_sprite(
        TEXT_LEFT_ON_BAND,
        "Press and hold the center of the cross",
        AREA512_QUATERNARY_SPRITE_TEXT_COLOR_INDEX
      );
      break;
    }

    draw_cross_on_band_sprite(
      cross.x,
      cross.y - band_index * BAND_HEIGHT
    );

    push_band_sprite(band_index);
  }
}

static bool
wait_for_cross_press(int *raw_x, int *raw_y) {
  int raw_x_samples[CALIBRATION_MAXIMUM_SAMPLE_COUNT];
  int raw_y_samples[CALIBRATION_MAXIMUM_SAMPLE_COUNT];
  int sample_count = 0;
  int consecutive_missed_poll_count = 0;
  int64_t wait_start_uptime_milliseconds = area512_read_uptime_ms();

  for (;;) {
    if (
      sample_count == 0 &&
      area512_read_uptime_ms() - wait_start_uptime_milliseconds >=
        CALIBRATION_TIMEOUT_MILLISECONDS
    )
      return false;

    int sample_raw_x;
    int sample_raw_y;

    if (area512_display_read_raw_touch_point(&sample_raw_x, &sample_raw_y)) {
      consecutive_missed_poll_count = 0;

      if (sample_count < CALIBRATION_MAXIMUM_SAMPLE_COUNT) {
        raw_x_samples[sample_count] = sample_raw_x;
        raw_y_samples[sample_count] = sample_raw_y;
        sample_count++;
      }
    } else if (sample_count > 0) {
      consecutive_missed_poll_count++;

      if (
        consecutive_missed_poll_count >= AREA512_MISSED_POLL_COUNT_TO_END_TOUCH
      ) {

        if (sample_count >= CALIBRATION_MINIMUM_SAMPLE_COUNT) {
          *raw_x = area512_sort_and_find_median(raw_x_samples, sample_count);
          *raw_y = area512_sort_and_find_median(raw_y_samples, sample_count);

          return true;
        }

        sample_count = 0;
        consecutive_missed_poll_count = 0;
      }
    }

    vTaskDelay(pdMS_TO_TICKS(POLL_INTERVAL_MILLISECONDS));
  }
}

static bool
measure_and_apply_touch_calibration(TouchSetting *touch_setting) {
  int window_width = area512_gfx_width();
  int window_height = area512_gfx_height();

  const ScreenPoint crosses[TOUCH_CALIBRATION_POINT_COUNT] = {
    {CALIBRATION_CROSS_INSET, CALIBRATION_CROSS_INSET},
    {window_width - 1 - CALIBRATION_CROSS_INSET, CALIBRATION_CROSS_INSET},
    {CALIBRATION_CROSS_INSET, window_height - 1 - CALIBRATION_CROSS_INSET},
    {
      window_width - 1 - CALIBRATION_CROSS_INSET,
      window_height - 1 - CALIBRATION_CROSS_INSET
    },
  };

  TouchCalibrationPoint calibration_points[TOUCH_CALIBRATION_POINT_COUNT];

  bool is_every_press_read = true;
  bool is_calibrated = false;

  area512_lock_touch_panel();

  for (int i = 0; i < TOUCH_CALIBRATION_POINT_COUNT; i++) {
    draw_calibration_screen(i, crosses[i]);

    if (
      !wait_for_cross_press(
        &calibration_points[i].raw_x,
        &calibration_points[i].raw_y
      )
    ) {

      is_every_press_read = false;

      break;
    }

    area512_display_convert_screen_to_touch_calibration_point(
      crosses[i].x,
      crosses[i].y,
      &calibration_points[i].panel_x,
      &calibration_points[i].panel_y
    );
  }

  if (is_every_press_read) {
    int panel_width;
    int panel_height;

    area512_display_read_touch_calibration_size(
      &panel_width,
      &panel_height
    );

    is_calibrated =
      area512_compute_touch_calibration_values(
        calibration_points,
        panel_width,
        panel_height,
        touch_setting->calibration_values
      );
  }

  if (is_calibrated) {
    area512_display_set_touch_calibration(touch_setting->calibration_values);
    touch_setting->has_calibration = true;
  }

  area512_unlock_touch_panel();

  area512_clear_touch_events();

  return is_calibrated;
}

static ScreenRectangle
build_distance_button_rectangle(
  const DistanceSetting *distance_setting,
  int button_screen_left
) {

  ScreenRectangle rectangle = {
    button_screen_left,
    BAND_INNER_SCREEN_TOP(distance_setting->band_index),
    DISTANCE_BUTTON_WIDTH,
    BAND_INNER_HEIGHT
  };

  return rectangle;
}

static void
draw_distance_band(const DistanceSetting *distance_setting) {
  char text[TEXT_BYTE_SIZE];

  ScreenButton decrease_button = {
    build_distance_button_rectangle(
      distance_setting,
      DISTANCE_DECREASE_BUTTON_SCREEN_LEFT
    ),
    "-"
  };

  ScreenButton increase_button = {
    build_distance_button_rectangle(
      distance_setting,
      DISTANCE_INCREASE_BUTTON_SCREEN_LEFT
    ),
    "+"
  };

  snprintf(
    text,
    sizeof(text),
    "%-11s %3dpx",
    distance_setting->label,
    distance_setting->read_distance()
  );

  clear_band_sprite();

  draw_text_on_band_sprite(
    TEXT_LEFT_ON_BAND,
    text,
    AREA512_QUATERNARY_SPRITE_TEXT_COLOR_INDEX
  );

  draw_button_on_band_sprite(
    distance_setting->band_index,
    &decrease_button
  );

  draw_button_on_band_sprite(
    distance_setting->band_index,
    &increase_button
  );

  push_band_sprite(distance_setting->band_index);
}

static void
draw_test_area(const char *gesture_name) {
  char text[TEXT_BYTE_SIZE];

  snprintf(text, sizeof(text), "Try here: %s", gesture_name);

  for (
    int band_index = TEST_AREA_TOP_BAND_INDEX;
    band_index <= TEST_AREA_TOP_BAND_INDEX + 1;
    band_index++
  ) {

    clear_band_sprite();
    draw_rectangle_on_band_sprite(band_index, &TEST_AREA_RECTANGLE);

    if (band_index == TEST_AREA_TOP_BAND_INDEX)
      draw_text_on_band_sprite(
        TEST_AREA_RECTANGLE.left + 8,
        text,
        AREA512_QUATERNARY_SPRITE_TEXT_COLOR_INDEX
      );

    push_band_sprite(band_index);
  }
}

static const char *
find_drag_gesture_name(
  int drag_axis,
  int horizontal_offset_from_touch_start,
  int vertical_offset_from_touch_start
) {

  if (drag_axis == AREA512_TOUCH_DRAG_AXIS_VERTICAL)
    return "SCROLL";

  switch (
    area512_find_touch_swipe_direction(
      drag_axis,
      horizontal_offset_from_touch_start,
      vertical_offset_from_touch_start
    )
  ) {

  case AREA512_TOUCH_SWIPE_RIGHT:
    return "SWIPE RIGHT";

  case AREA512_TOUCH_SWIPE_LEFT:
    return "SWIPE LEFT";

  default:
    return "NONE";
  }
}

static void
change_and_draw_distance_at(int touch_screen_x, int touch_screen_y) {
  for (int i = 0; i < DISTANCE_SETTING_COUNT; i++) {
    const DistanceSetting *distance_setting = &DISTANCE_SETTINGS[i];

    ScreenRectangle decrease_rectangle =
      build_distance_button_rectangle(
        distance_setting,
        DISTANCE_DECREASE_BUTTON_SCREEN_LEFT
      );

    ScreenRectangle increase_rectangle =
      build_distance_button_rectangle(
        distance_setting,
        DISTANCE_INCREASE_BUTTON_SCREEN_LEFT
      );

    int distance = distance_setting->read_distance();

    if (
      is_touch_point_in_rectangle(
        &decrease_rectangle,
        touch_screen_x,
        touch_screen_y
      )
    )
      distance -= DISTANCE_STEP;
    else if (
      is_touch_point_in_rectangle(
        &increase_rectangle,
        touch_screen_x,
        touch_screen_y
      )
    )
      distance += DISTANCE_STEP;
    else
      continue;

    if (distance < MINIMUM_DISTANCE)
      distance = MINIMUM_DISTANCE;

    distance_setting->set_distance(distance);
    draw_distance_band(distance_setting);

    return;
  }
}

static bool
edit_touch_distances_and_check_save_tapped(void) {
  bool is_dragging = false;
  bool is_drag_started_in_test_area = false;
  int touch_start_screen_x = 0;
  int touch_start_screen_y = 0;
  int drag_axis = AREA512_TOUCH_DRAG_AXIS_NONE;

  clear_screen();

  draw_text_band(
    DISTANCE_TITLE_BAND_INDEX,
    "TOUCH SETTING 2/2  DRAG / SWIPE",
    AREA512_QUATERNARY_SPRITE_EMPHASIS_COLOR_INDEX
  );

  for (int i = 0; i < DISTANCE_SETTING_COUNT; i++)
    draw_distance_band(&DISTANCE_SETTINGS[i]);

  draw_test_area("-");

  clear_band_sprite();

  draw_button_on_band_sprite(
    CANCEL_AND_SAVE_BUTTON_BAND_INDEX,
    &CANCEL_BUTTON
  );

  draw_button_on_band_sprite(
    CANCEL_AND_SAVE_BUTTON_BAND_INDEX,
    &SAVE_BUTTON
  );

  push_band_sprite(CANCEL_AND_SAVE_BUTTON_BAND_INDEX);

  for (;;) {
    int touch_event_kind;
    int touch_screen_x;
    int touch_screen_y;
    int64_t touch_event_uptime_milliseconds;

    while (
      area512_read_touch_event(
        &touch_event_kind,
        &touch_screen_x,
        &touch_screen_y,
        &touch_event_uptime_milliseconds
      )
    ) {

      switch (touch_event_kind) {
      case AREA512_TOUCH_DRAG:
        if (!is_dragging) {
          is_dragging = true;

          is_drag_started_in_test_area =
            is_touch_point_in_rectangle(
              &TEST_AREA_RECTANGLE,
              touch_screen_x,
              touch_screen_y
            );

          touch_start_screen_x = touch_screen_x;
          touch_start_screen_y = touch_screen_y;
          drag_axis = AREA512_TOUCH_DRAG_AXIS_NONE;

        } else if (drag_axis == AREA512_TOUCH_DRAG_AXIS_NONE) {
          drag_axis =
            area512_find_touch_drag_axis(
              touch_screen_x - touch_start_screen_x,
              touch_screen_y - touch_start_screen_y
            );
        }

        break;

      case AREA512_TOUCH_RELEASE:
        if (is_drag_started_in_test_area)
          draw_test_area(
            find_drag_gesture_name(
              drag_axis,
              touch_screen_x - touch_start_screen_x,
              touch_screen_y - touch_start_screen_y
            )
          );

        is_dragging = false;
        is_drag_started_in_test_area = false;

        break;

      case AREA512_TOUCH_TAP:
        if (
          is_touch_point_in_rectangle(
            &TEST_AREA_RECTANGLE,
            touch_screen_x,
            touch_screen_y
          )
        )
          draw_test_area("TAP");

        else if (
          is_touch_point_in_rectangle(
            &SAVE_BUTTON.rectangle,
            touch_screen_x,
            touch_screen_y
          )
        )
          return true;

        else if (
          is_touch_point_in_rectangle(
            &CANCEL_BUTTON.rectangle,
            touch_screen_x,
            touch_screen_y
          )
        )
          return false;
        else
          change_and_draw_distance_at(touch_screen_x, touch_screen_y);

        break;
      }
    }

    vTaskDelay(pdMS_TO_TICKS(POLL_INTERVAL_MILLISECONDS));
  }
}

static bool
apply_and_save_touch_setting_with_current_distances(
  TouchSetting *touch_setting
) {

  area512_read_current_touch_distances(touch_setting);
  area512_apply_touch_setting(touch_setting);

  return area512_save_touch_setting(touch_setting);
}

static void
draw_message_and_wait(int band_index, const char *message) {
  draw_text_band(
    band_index,
    message,
    AREA512_QUATERNARY_SPRITE_EMPHASIS_COLOR_INDEX
  );
  vTaskDelay(pdMS_TO_TICKS(MESSAGE_HOLD_MILLISECONDS));
}

void
area512_run_touch_setting(void) {
  s_band_sprite =
    area512_quaternary_sprite_new_with_font_size(
      area512_gfx_width(),
      BAND_HEIGHT,
      BAND_FONT_SIZE
    );

  if (s_band_sprite == NULL)
    return;

  TouchSetting previous_touch_setting;
  TouchSetting touch_setting;

  area512_read_current_touch_setting(&previous_touch_setting);
  touch_setting = previous_touch_setting;

  area512_clear_touch_events();

  if (!measure_and_apply_touch_calibration(&touch_setting)) {
    clear_screen();

    draw_message_and_wait(
      CALIBRATION_STOPPED_MESSAGE_BAND_INDEX,
      "Calibration stopped. Nothing was changed"
    );

  } else {
    if (!edit_touch_distances_and_check_save_tapped())
      area512_apply_touch_setting(&previous_touch_setting);

    else if (
      !apply_and_save_touch_setting_with_current_distances(&touch_setting)
    )
      draw_message_and_wait(
        SAVE_ERROR_MESSAGE_BAND_INDEX,
        "Could not write etc/touch"
      );
  }

  area512_clear_touch_events();
  area512_sprite_delete(s_band_sprite);
  s_band_sprite = NULL;
}
