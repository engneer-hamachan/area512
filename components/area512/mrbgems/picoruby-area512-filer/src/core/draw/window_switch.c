#include "core/draw/window_switch.h"

#include "area512_hal.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WINDOW_SWITCH_ON_DURATION_MILLISECONDS 320
#define WINDOW_SWITCH_OFF_DURATION_MILLISECONDS 320
#define SWITCH_ON_LINE_GROW_PROGRESS_END 0.5f

#define LIT_LINE_HEIGHT 2
#define LIT_LINE_COLOR 0xFFF6E6

static float
ease_out_cubic(float progress) {
  return 1.0f - powf(1.0f - progress, 3.0f);
}

static int
lerp_int(int from, int to, float fraction) {
  return (int)lroundf((float)from + (float)(to - from) * fraction);
}

static WindowRect
compute_centered_lit_line_rect(WindowRect window_rect, int line_width) {
  int center_left = window_rect.left + window_rect.width / 2;
  int center_top = window_rect.top + window_rect.height / 2;

  return (WindowRect){
    .left = center_left - line_width / 2,
    .top = center_top - LIT_LINE_HEIGHT / 2,
    .width = line_width,
    .height = LIT_LINE_HEIGHT,
  };
}

static WindowRect
compute_centered_body_rect(WindowRect window_rect, int body_height) {
  int center_top = window_rect.top + window_rect.height / 2;

  return (WindowRect){
    .left = window_rect.left,
    .top = center_top - body_height / 2,
    .width = window_rect.width,
    .height = body_height,
  };
}

static WindowSwitchShape
compute_switch_on_shape(WindowRect window_rect, float progress) {
  WindowSwitchShape shape;

  memset(&shape, 0, sizeof shape);

  if (progress < SWITCH_ON_LINE_GROW_PROGRESS_END) {
    int line_width =
      lerp_int(
        0,
        window_rect.width,
        progress / SWITCH_ON_LINE_GROW_PROGRESS_END
      );

    shape.lit_line_rect =
      compute_centered_lit_line_rect(window_rect, line_width);

    shape.draws_lit_line = 1;

    return shape;
  }

  float eased_progress =
    ease_out_cubic(
      (progress - SWITCH_ON_LINE_GROW_PROGRESS_END) /
      (1.0f - SWITCH_ON_LINE_GROW_PROGRESS_END)
    );

  int body_height =
    lerp_int(
      LIT_LINE_HEIGHT,
      window_rect.height,
      eased_progress
    );

  shape.lit_line_rect =
    compute_centered_lit_line_rect(window_rect, window_rect.width);

  shape.draws_lit_line = eased_progress < 1.0f;
  shape.has_body = 1;
  shape.body_rect = compute_centered_body_rect(window_rect, body_height);

  return shape;
}

void
draw_window_switch_shape(Filer *filer) {
  if (filer->window_switch_shape.has_body) {
    area512_sprite_fill_rect(
      filer->screen,
      filer->window_switch_shape.body_rect.left,
      filer->window_switch_shape.body_rect.top,
      filer->window_switch_shape.body_rect.width,
      filer->window_switch_shape.body_rect.height,
      area512_theme_background_color()
    );
  }

  if (filer->window_switch_shape.draws_lit_line) {
    area512_sprite_fill_rect(
      filer->screen,
      filer->window_switch_shape.lit_line_rect.left,
      filer->window_switch_shape.lit_line_rect.top,
      filer->window_switch_shape.lit_line_rect.width,
      filer->window_switch_shape.lit_line_rect.height,
      LIT_LINE_COLOR
    );
  }
}

static void
draw_body_rows_on_display(
  WindowRect window_rect,
  const uint8_t *window_image_bitmap,
  int top,
  int height
) {

  if (height <= 0)
    return;

  if (window_image_bitmap == NULL) {
    area512_gfx_fill_rect(
      window_rect.left,
      top,
      window_rect.width,
      height,
      area512_theme_background_color()
    );

    return;
  }

  area512_gfx_draw_theme_bitmap(
    window_rect.left,
    top,
    window_image_bitmap +
    (size_t)(top - window_rect.top) * ((window_rect.width + 7) / 8),
    window_rect.width,
    height
  );
}

static void
draw_switch_on_frame_on_display(
  WindowRect window_rect,
  const uint8_t *window_image_bitmap,
  const WindowSwitchShape *previous_shape,
  const WindowSwitchShape *current_shape
) {

  if (current_shape->draws_lit_line)
    area512_gfx_fill_rect(
      current_shape->lit_line_rect.left,
      current_shape->lit_line_rect.top,
      current_shape->lit_line_rect.width,
      current_shape->lit_line_rect.height,
      LIT_LINE_COLOR
    );

  if (current_shape->has_body && !previous_shape->has_body) {
    draw_body_rows_on_display(
      window_rect,
      window_image_bitmap,
      current_shape->body_rect.top,
      current_shape->lit_line_rect.top - current_shape->body_rect.top
    );

    draw_body_rows_on_display(
      window_rect,
      window_image_bitmap,
      current_shape->lit_line_rect.top + current_shape->lit_line_rect.height,
      current_shape->body_rect.top + current_shape->body_rect.height -
      current_shape->lit_line_rect.top - current_shape->lit_line_rect.height
    );
  }

  if (current_shape->has_body && previous_shape->has_body) {
    draw_body_rows_on_display(
      window_rect,
      window_image_bitmap,
      current_shape->body_rect.top,
      previous_shape->body_rect.top - current_shape->body_rect.top
    );

    draw_body_rows_on_display(
      window_rect,
      window_image_bitmap,
      previous_shape->body_rect.top + previous_shape->body_rect.height,
      current_shape->body_rect.top + current_shape->body_rect.height -
      previous_shape->body_rect.top - previous_shape->body_rect.height
    );
  }

  if (current_shape->has_body && !current_shape->draws_lit_line)
    draw_body_rows_on_display(
      window_rect,
      window_image_bitmap,
      current_shape->lit_line_rect.top,
      current_shape->lit_line_rect.height
    );
}

static void
play_window_switch_on(Filer *filer) {
  if (!filer->row)
    return;

  WindowRect window_rect = compute_window_rect(filer);
  WindowSwitchShape previous_shape;

  memset(&previous_shape, 0, sizeof previous_shape);

  uint32_t start_uptime_milliseconds = area512_uptime_milliseconds();

  for (;;) {
    uint32_t elapsed_milliseconds =
      area512_uptime_milliseconds() - start_uptime_milliseconds;

    float progress =
      (float)elapsed_milliseconds /
      (float)WINDOW_SWITCH_ON_DURATION_MILLISECONDS;

    if (progress > 1.0f)
      progress = 1.0f;

    WindowSwitchShape current_shape =
      compute_switch_on_shape(window_rect, progress);

    if (memcmp(&current_shape, &previous_shape, sizeof current_shape) != 0)
      draw_switch_on_frame_on_display(
        window_rect,
        filer->window_image_bitmap,
        &previous_shape,
        &current_shape
      );

    previous_shape = current_shape;

    if (progress >= 1.0f)
      break;
  }
}

static void
draw_switch_off_frame(Filer *filer, WindowRect window_rect, float progress) {
  WindowSwitchShape current_shape =
    compute_switch_on_shape(window_rect, 1.0f - progress);

  if (
    filer->draws_window_switch_shape &&
    memcmp(
      &current_shape,
      &filer->window_switch_shape,
      sizeof current_shape
    ) == 0
  )
    return;

  filer->window_switch_shape = current_shape;
  filer->draws_window_switch_shape = 1;

  draw_all(filer);
}

static void
play_window_switch_off(Filer *filer) {
  if (!filer->row)
    return;

  WindowRect window_rect = compute_window_rect(filer);

  draw_switch_off_frame(filer, window_rect, 0.0f);

  uint32_t start_uptime_milliseconds = area512_uptime_milliseconds();

  for (;;) {
    uint32_t elapsed_milliseconds =
      area512_uptime_milliseconds() - start_uptime_milliseconds;

    float progress =
      (float)elapsed_milliseconds /
      (float)WINDOW_SWITCH_OFF_DURATION_MILLISECONDS;

    if (progress > 1.0f)
      progress = 1.0f;

    draw_switch_off_frame(filer, window_rect, progress);

    if (progress >= 1.0f)
      break;
  }

  filer->draws_window_switch_shape = 0;
}

static void
mark_body_rows_changed(
  const WindowSwitchShape *previous_shape,
  const WindowSwitchShape *current_shape,
  uint8_t *rows
) {

  if (!previous_shape->has_body || !current_shape->has_body) {
    if (previous_shape->has_body)
      memset(
        rows + previous_shape->body_rect.top,
        1,
        previous_shape->body_rect.height
      );

    if (current_shape->has_body)
      memset(
        rows + current_shape->body_rect.top,
        1,
        current_shape->body_rect.height
      );

    return;
  }

  int top_strip_top =
    previous_shape->body_rect.top < current_shape->body_rect.top
      ? previous_shape->body_rect.top
      : current_shape->body_rect.top;

  int top_strip_height =
    abs(previous_shape->body_rect.top - current_shape->body_rect.top);

  int bottom_strip_top =
    previous_shape->body_rect.top + previous_shape->body_rect.height <
    current_shape->body_rect.top + current_shape->body_rect.height
      ? previous_shape->body_rect.top + previous_shape->body_rect.height
      : current_shape->body_rect.top + current_shape->body_rect.height;

  int bottom_strip_height =
    abs(
      previous_shape->body_rect.top + previous_shape->body_rect.height -
      current_shape->body_rect.top - current_shape->body_rect.height
    );

  memset(rows + top_strip_top, 1, top_strip_height);
  memset(rows + bottom_strip_top, 1, bottom_strip_height);
}

void
mark_window_switch_rows_changed(const Filer *filer, uint8_t *rows) {
  memset(
    rows + filer->window_switch_shape.lit_line_rect.top,
    1,
    filer->window_switch_shape.lit_line_rect.height
  );

  memset(
    rows + filer->drawn.window_switch_shape.lit_line_rect.top,
    1,
    filer->drawn.window_switch_shape.lit_line_rect.height
  );

  mark_body_rows_changed(
    &filer->drawn.window_switch_shape,
    &filer->window_switch_shape,
    rows
  );
}

void
load_window_image_bitmap(Filer *filer) {
  char image_path[CURRENT_DIRECTORY_MAX + NAME_MAX + 16];
  FileEntry *entry = fetch_selected_entry(filer);

  if (filer->action_target_path[0])
    snprintf(
      image_path,
      sizeof image_path,
      "%s/image.h",
      filer->action_target_path
    );

  else if (entry && strcmp(filer->current_directory, "/") == 0)
    snprintf(image_path, sizeof image_path, "/%s/image.h", entry->name);

  else if (entry)
    snprintf(
      image_path,
      sizeof image_path,
      "%s/%s/image.h",
      filer->current_directory,
      entry->name
    );

  else
    return;

  WindowRect window_rect = compute_window_rect(filer);

  size_t window_image_bitmap_size =
    (size_t)((window_rect.width + 7) / 8) * window_rect.height;

  filer->window_image_bitmap = (uint8_t *)malloc(window_image_bitmap_size);

  if (filer->window_image_bitmap == NULL)
    return;

  if (
    !area512_gfx_load_header_image(
      image_path,
      filer->window_image_bitmap,
      window_image_bitmap_size
    )
  )
    free_window_image_bitmap(filer);
}

void
free_window_image_bitmap(Filer *filer) {
  free(filer->window_image_bitmap);

  filer->window_image_bitmap = NULL;
}

void
ensure_window_open_with_switch_on(Filer *filer) {
  if (filer->is_window_open)
    return;

  play_window_switch_on(filer);

  filer->is_window_open = 1;
}

void
ensure_window_closed_with_switch_off(Filer *filer) {
  if (!filer->is_window_open)
    return;

  play_window_switch_off(filer);

  filer->is_window_open = 0;
}
