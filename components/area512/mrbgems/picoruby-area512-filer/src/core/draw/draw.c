#include "core/draw/draw.h"

#include <string.h>

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

  // center the box (panel_info.count+1 rows) vertically in the list area
  filer->panel_top_row = (filer->rows_visible - (panel_info.count + 1)) / 2;

  if (filer->panel_top_row < 0)
    filer->panel_top_row = 0;

  uint8_t changed_rows[filer->height];
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
      filer->has_background_image = area512_screen_draw_rgb565(
        filer->screen,
        area512_theme_background_image(),
        filer->message,
        sizeof(filer->message)
      );
      draw_background = filer->has_background_image;

      if (!draw_background) {
        background_failed = 1;
        memset(
          changed_rows + filer->bar1_y,
          1,
          filer->height - filer->bar1_y
        );
      }
    }

    int row = 0;

    while (row < filer->rows_visible) {
      draw_entry(
        filer,
        filer->list_top + row * ROW_HEIGHT,
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

    area512_screen_push_region(filer->screen);
  }

  area512_sprite_delete(filer->screen);
  filer->screen = 0;
  filer->full_redraw = 0;
  save_draw_state(filer, &panel_info);
  filer->drawn.valid = !background_failed;
}
