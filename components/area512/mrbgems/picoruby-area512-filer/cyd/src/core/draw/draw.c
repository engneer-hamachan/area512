#include "core/draw/draw.h"

#include <string.h>

int
compute_corner_inset(int dy, int radius) {
  if (dy <= 0)
    return 0;

  int dx = radius;

  while (dx * dx + dy * dy > radius * radius)
    dx--;

  return radius - dx;
}

static void
blend_action_bar_background(Filer *filer) {
  int left = FRAME_INSET + 2;
  int bottom = CLOSE_Y + FRAME_BOTTOM_HEIGHT - 2;
  int radius = FRAME_RADIUS - 2;

  for (int y = BAR_Y; y < bottom; y++) {
    int inset = compute_corner_inset(y - (bottom - radius - 1), radius);

    area512_sprite_blend_rect(
      filer->screen,
      left + inset,
      y,
      SCREEN_WIDTH - (left + inset) * 2,
      1,
      area512_theme_background_color(),
      85
    );
  }
}

void
draw_all(Filer *filer) {
  if (!filer->row)
    return;

  filer->screen = area512_screen_new(FILER_FONT_SIZE);

  if (!filer->screen)
    return;

  adjust_scroll(filer);

  if (!filer->has_key_wait_timed_out)
    build_panel_info(&filer->panel_info);

  filer->pressed_chip_index = find_pressed_chip_index(filer);

  uint8_t changed_rows[SCREEN_HEIGHT];

  find_changed_rows(filer, changed_rows);

  int draw_background = 1;
  int background_failed = 0;

  int changed_first_row = 0;

  while (changed_first_row < SCREEN_HEIGHT) {
    if (!changed_rows[changed_first_row]) {
      changed_first_row++;
      continue;
    }

    int changed_row_count = 0;

    while (
      changed_first_row + changed_row_count < SCREEN_HEIGHT &&
      changed_rows[changed_first_row + changed_row_count] &&
      changed_row_count < AREA512_SCREEN_REGION_ROW_COUNT
    )
      changed_row_count++;

    if (
      !area512_screen_begin_region_at(
        filer->screen,
        changed_first_row,
        changed_row_count
      )
    )
      break;

    area512_sprite_fill(filer->screen, area512_theme_background_color());
    filer->has_background_image = 0;

    if (draw_background) {
      filer->has_background_image =
        area512_screen_draw_rgb565(
          filer->screen,
          area512_theme_background_image(),
          filer->message,
          sizeof(filer->message)
        );

      draw_background = filer->has_background_image;

      if (!draw_background) {
        background_failed = 1;
        memset(changed_rows + BAR_Y, 1, SCREEN_HEIGHT - BAR_Y);
      }
    }

    if (filer->has_background_image)
      blend_action_bar_background(filer);

    int row = 0;

    while (row < ROWS_VISIBLE) {
      draw_entry(
        filer,
        HEADER_HEIGHT + row * ROW_HEIGHT,
        filer->top + row,
        row,
        &filer->panel_info
      );

      row++;
    }

    draw_action_bar(filer);
    draw_frame(filer);
    draw_header(filer);
    draw_message_above_soft_keyboard(filer);

    if (area512_is_filer_window_open_animation_running())
      area512_draw_filer_window_open_animation(
        filer->screen,
        find_window_left(filer) - 1,
        find_window_top(filer) - 1,
        AREA512_WINDOW_WIDTH + 2,
        AREA512_WINDOW_HEIGHT + 2
      );
    else if (filer->terminal || filer->is_window_open)
      draw_window_box(
        filer->screen,
        find_window_left(filer) - 1,
        find_window_top(filer) - 1,
        AREA512_WINDOW_WIDTH + 2,
        AREA512_WINDOW_HEIGHT + 2
      );

    if (area512_is_filer_window_close_animation_running())
      area512_draw_filer_window_close_animation(
        filer->screen,
        filer->closing_window_left - 1,
        filer->closing_window_top - 1,
        AREA512_WINDOW_WIDTH + 2,
        AREA512_WINDOW_HEIGHT + 2
      );

    area512_screen_push_region(filer->screen);

    changed_first_row += changed_row_count;
  }

  area512_sprite_delete(filer->screen);

  filer->screen = 0;
  filer->full_redraw = 0;

  save_draw_state(filer);

  filer->drawn.valid = !background_failed;
}

int
find_window_left(const Filer *filer) {
  return filer->is_window_open ? filer->app_window_left : WINDOW_LEFT;
}

static int
find_window_top_without_soft_keyboard(const Filer *filer) {
  return filer->is_window_open ? filer->app_window_top_without_soft_keyboard
                               : WINDOW_TOP_WITHOUT_SOFT_KEYBOARD;
}

int
find_window_top(const Filer *filer) {
  return area512_gfx_find_window_top(
    find_window_top_without_soft_keyboard(filer),
    AREA512_WINDOW_HEIGHT
  );
}

void
area512_filer_set_gfx_window(const Filer *filer) {
  area512_gfx_set_window(
    find_window_left(filer),
    find_window_top_without_soft_keyboard(filer),
    AREA512_WINDOW_WIDTH,
    AREA512_WINDOW_HEIGHT
  );
}

void
area512_filer_open_window(Filer *filer) {
  area512_gfx_set_window(0, 0, 0, 0);

  draw_all(filer);

  area512_filer_set_gfx_window(filer);
}
