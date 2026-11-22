#pragma once

void area512_sprite_set_title_font(void *p);
void area512_sprite_round_rect(
  void *p,
  int x,
  int y,
  int w,
  int h,
  int corner_radius,
  uint32_t color
);

#define AREA512_WINDOW_WIDTH 240
#define AREA512_WINDOW_HEIGHT 135

void area512_gfx_set_window(int x, int y, int width, int height);
void area512_gfx_show_bitmap(const void *bitmap, int line_delay_milliseconds);
int area512_gfx_window_left(void);
int area512_gfx_window_top(void);
int area512_gfx_read_window_top_without_soft_keyboard(void);
void area512_gfx_move_window(
  int screen_left,
  int screen_top_without_soft_keyboard
);
int area512_gfx_find_window_top(int window_top, int window_height);
void area512_gfx_set_subwindow(int x, int y, int width, int height);
void area512_gfx_push_subwindow(void *sprite);

// The soft keyboard draws the app screen again when it opens or closes,
// because the window moves.
void area512_gfx_set_app_background_drawer(
  void (*draw_app_background)(void *context),
  void *context
);
void area512_gfx_show_app_screen(void);
void area512_gfx_hide_app_screen(void);
void area512_gfx_draw_app_screen(void);

#define AREA512_SOFT_KEYBOARD_TOP 144

void area512_gfx_clear_soft_keyboard_area(void);
void area512_gfx_draw_soft_keyboard_key(
  int x,
  int y,
  int width,
  int height,
  const char *label,
  bool is_pressed
);

void area512_open_soft_keyboard(void);
void area512_close_soft_keyboard(void);
bool area512_is_soft_keyboard_open(void);

#define AREA512_TOUCH_TAP 0
#define AREA512_TOUCH_DRAG 1
#define AREA512_TOUCH_RELEASE 2

bool area512_read_touch_event(
  int *kind,
  int *touch_screen_x,
  int *touch_screen_y,
  int64_t *event_uptime_ms
);
bool area512_read_touch_start_point(int *touch_screen_x, int *touch_screen_y);

// One missed read during a press must not split it into two touches.
#define AREA512_MISSED_POLL_COUNT_TO_END_TOUCH 3

#define AREA512_TOUCH_DRAG_AXIS_NONE 0
#define AREA512_TOUCH_DRAG_AXIS_HORIZONTAL 1
#define AREA512_TOUCH_DRAG_AXIS_VERTICAL 2

int area512_read_touch_drag_start_distance(void);
void area512_set_touch_drag_start_distance(int drag_start_distance);
int area512_read_touch_drag_axis_decision_distance(void);
void area512_set_touch_drag_axis_decision_distance(
  int drag_axis_decision_distance
);
int area512_read_touch_swipe_distance(void);
void area512_set_touch_swipe_distance(int swipe_distance);
int area512_find_touch_drag_axis(
  int horizontal_distance,
  int vertical_distance
);
#define AREA512_TOUCH_SWIPE_NONE 0
#define AREA512_TOUCH_SWIPE_LEFT 1
#define AREA512_TOUCH_SWIPE_RIGHT 2

int area512_find_touch_swipe_direction(
  int drag_axis,
  int horizontal_distance,
  int vertical_distance
);

// The same task must call area512_unlock_touch_panel.
void area512_lock_touch_panel(void);
void area512_unlock_touch_panel(void);

void area512_load_and_apply_touch_setting(void);
void area512_run_touch_setting(void);
int64_t area512_read_uptime_ms(void);
int area512_sort_and_find_median(int *values, int value_count);
void area512_clear_touch_events(void);
