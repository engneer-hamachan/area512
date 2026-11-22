#include "area512_hal.h"

#include "area512_animation_timer.h"
#include "filer_window_box.h"

#if defined(AREA512_EXT_DISPLAY) || defined(AREA512_CYD)
#define WINDOW_CLOSE_DURATION_MS 1080
#else
#define WINDOW_CLOSE_DURATION_MS 800
#endif
#define SQUEEZE_END_PROGRESS 0.4f
#define LIT_LINE_SHRINK_START_PROGRESS 0.6f

static AnimationTimer s_window_close_animation_timer = {
  .duration_ms = WINDOW_CLOSE_DURATION_MS,
};

static void
draw_squeezing_window_box(
  void *sprite,
  int left,
  int top,
  int width,
  int height,
  float progress
) {

  float eased_open_progress =
    area512_compute_ease_out_progress(
      area512_compute_progress_between(
        1.0f - progress,
        1.0f - SQUEEZE_END_PROGRESS,
        1.0f
      )
    );

  int body_height =
    area512_interpolate_linearly(LIT_LINE_HEIGHT, height, eased_open_progress);

  area512_draw_filer_window_box(
    sprite,
    left,
    top + height / 2 - body_height / 2,
    width,
    body_height
  );

  area512_draw_filer_window_lit_line(
    sprite,
    left,
    top,
    width,
    height,
    width,
    (int)((1.0f - eased_open_progress) * 100.0f)
  );
}

static void
draw_shrinking_lit_line(
  void *sprite,
  int left,
  int top,
  int width,
  int height,
  float progress
) {

  int lit_line_width =
    area512_interpolate_linearly(
      width,
      0,
      area512_compute_progress_between(
        progress,
        LIT_LINE_SHRINK_START_PROGRESS,
        1.0f
      )
    );

  area512_draw_filer_window_lit_line(
    sprite,
    left,
    top,
    width,
    height,
    lit_line_width,
    100
  );
}

void
area512_start_filer_window_close_animation(
  void (*draw_screen)(void *context),
  void *context
) {

  s_window_close_animation_timer =
    area512_start_animation_timer(WINDOW_CLOSE_DURATION_MS);

  while (area512_is_filer_window_close_animation_running())
    draw_screen(context);
}

bool
area512_is_filer_window_close_animation_running(void) {
  return !area512_is_animation_over(
    s_window_close_animation_timer,
    area512_read_uptime_ms()
  );
}

void
area512_draw_filer_window_close_animation(
  void *sprite,
  int left,
  int top,
  int width,
  int height
) {

  float progress =
    area512_compute_animation_progress(
      s_window_close_animation_timer,
      area512_read_uptime_ms()
    );

  if (progress < SQUEEZE_END_PROGRESS)
    draw_squeezing_window_box(sprite, left, top, width, height, progress);
  else
    draw_shrinking_lit_line(sprite, left, top, width, height, progress);
}
