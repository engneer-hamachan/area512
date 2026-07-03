#include "core/draw/draw.h"

void
draw_all(Filer *filer) {
  if (!filer->row)
    return;

  if (filer->full_redraw) {
    area512_gfx_fill_screen(COLOR_BACKGROUND);
    filer->full_redraw = 0;
  }

  draw_header(filer);
  adjust_scroll(filer);

  PanelInfo panel_info;
  build_panel_info(&panel_info);

  // center the box (panel_info.count+1 rows) vertically in the list area
  filer->panel_top_row = (filer->rows_visible - (panel_info.count + 1)) / 2;

  if (filer->panel_top_row < 0)
    filer->panel_top_row = 0;

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
  draw_close(filer);
}
