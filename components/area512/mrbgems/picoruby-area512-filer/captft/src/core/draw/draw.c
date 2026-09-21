#include "core/draw/draw.h"

#include <string.h>

#define WINDOW_LEFT ((SCREEN_WIDTH - AREA512_WINDOW_WIDTH) / 2)
#define WINDOW_TOP ((FRAME_TOP + BAR1_Y - AREA512_WINDOW_HEIGHT) / 2)

static void
draw_window_frame(Filer *filer) {
  area512_sprite_fill_rect(
    filer->screen,
    WINDOW_LEFT - 1,
    WINDOW_TOP - 1,
    AREA512_WINDOW_WIDTH + 2,
    AREA512_WINDOW_HEIGHT + 2,
    area512_theme_border_color()
  );

  area512_sprite_fill_rect(
    filer->screen,
    WINDOW_LEFT,
    WINDOW_TOP,
    AREA512_WINDOW_WIDTH,
    AREA512_WINDOW_HEIGHT,
    area512_theme_background_color()
  );
}

static int
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

  for (int y = BAR1_Y; y < bottom; y++) {
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

  PanelInfo panel_info;
  build_panel_info(&panel_info);

  uint8_t changed_rows[SCREEN_HEIGHT];

  find_changed_rows(filer, &panel_info, changed_rows);

  int draw_background = 1;
  int background_failed = 0;

  while (area512_screen_begin_region(filer->screen)) {
    int top = area512_screen_region_top(filer->screen);
    int bottom = area512_screen_region_bottom(filer->screen);
    int changed = 0;

    for (int row = top; row < bottom; row++) {
      if (changed_rows[row]) {
        changed = 1;
        break;
      }
    }

    if (!changed)
      continue;

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
        memset(changed_rows + BAR1_Y, 1, SCREEN_HEIGHT - BAR1_Y);
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
        &panel_info
      );

      row++;
    }

    draw_primary_action_bar(filer);
    draw_secondary_action_bar(filer);
    draw_frame(filer);
    draw_header(filer);

    if (filer->draws_window_frame)
      draw_window_frame(filer);

    area512_screen_push_region(filer->screen);
  }

  area512_sprite_delete(filer->screen);

  filer->screen = 0;
  filer->full_redraw = 0;

  save_draw_state(filer, &panel_info);

  filer->drawn.valid = !background_failed;
}

void
area512_filer_open_window(Filer *filer) {
  area512_gfx_set_window(0, 0, 0, 0);

  filer->draws_window_frame = 1;

  draw_all(filer);

  filer->draws_window_frame = 0;

  area512_gfx_set_window(
    WINDOW_LEFT,
    WINDOW_TOP,
    AREA512_WINDOW_WIDTH,
    AREA512_WINDOW_HEIGHT
  );
}
