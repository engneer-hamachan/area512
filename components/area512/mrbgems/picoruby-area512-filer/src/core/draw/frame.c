#include "core/draw/draw.h"

#include <stdio.h>
#include <string.h>

// Double side walls; top!=0 extends down from the header rule to form corners.
void
draw_walls(Filer *filer, int top) {
  int outer_top = top ? (HALF_ROW_HEIGHT - 1) : 0;
  int inner_top = top ? (HALF_ROW_HEIGHT + 1) : 0;

  area512_sprite_line(filer->row, 0, outer_top, 0, ROW_HEIGHT - 1, COLOR_GREEN);
  area512_sprite_line(filer->row, 2, inner_top, 2, ROW_HEIGHT - 1, COLOR_FAINT);

  area512_sprite_line(
    filer->row,
    filer->width - 1,
    outer_top,
    filer->width - 1,
    ROW_HEIGHT - 1,
    COLOR_GREEN
  );

  area512_sprite_line(
    filer->row,
    filer->width - 3,
    inner_top,
    filer->width - 3,
    ROW_HEIGHT - 1,
    COLOR_FAINT
  );
}

static void
draw_rule(Filer *filer, int outer_y, int inner_y) {
  area512_sprite_line(
    filer->row,
    0,
    outer_y,
    filer->width - 1,
    outer_y,
    COLOR_GREEN
  );

  area512_sprite_line(
    filer->row,
    2,
    inner_y,
    filer->width - 3,
    inner_y,
    COLOR_FAINT
  );
}

// Draw text over a border, clearing the rule behind it first.
static void
edge_text(Filer *filer, int x, const char *text, uint32_t color) {
  int width = (int)strlen(text) * FILER_CHAR_WIDTH;

  area512_sprite_fill_rect(
    filer->row,
    x - 1,
    0,
    width + 2,
    ROW_HEIGHT,
    COLOR_BACKGROUND
  );

  area512_sprite_text(filer->row, x, 0, text, color);
}

void
draw_header(Filer *filer) {
  area512_sprite_fill(filer->row, COLOR_BACKGROUND);
  draw_walls(filer, 1);
  draw_rule(filer, HALF_ROW_HEIGHT - 1, HALF_ROW_HEIGHT + 1);

  const char *brand = " AREA 512 ";
  int brand_x = (filer->width - (int)strlen(brand) * FILER_CHAR_WIDTH) / 2;

  edge_text(filer, brand_x, brand, COLOR_GOLD);

  char count[24];
  snprintf(count, sizeof count, " %d/%d ", filer->index + 1, filer->count);

  edge_text(
    filer,
    filer->width - 6 - (int)strlen(count) * FILER_CHAR_WIDTH,
    count,
    COLOR_GOLD
  );

  int available = (brand_x - 6) / FILER_CHAR_WIDTH;

  if (available > 1) {
    char path[CURRENT_DIRECTORY_MAX + 2];
    char fitted_path[LINE_MAX];

    snprintf(path, sizeof path, " %s ", filer->current_directory);

    fit_string(fitted_path, sizeof fitted_path, path, available);

    edge_text(filer, 4, fitted_path, COLOR_DIM);
  }

  area512_sprite_push(filer->row, 0, 0);
}

// Close the frame bottom with a double rule.
void
draw_close(Filer *filer) {
  area512_sprite_fill(filer->row, COLOR_BACKGROUND);
  area512_sprite_line(filer->row, 0, 0, 0, 2, COLOR_GREEN);

  area512_sprite_line(
    filer->row,
    filer->width - 1,
    0,
    filer->width - 1,
    2,
    COLOR_GREEN
  );

  area512_sprite_line(filer->row, 0, 2, filer->width - 1, 2, COLOR_GREEN);
  area512_sprite_line(filer->row, 2, 0, filer->width - 3, 0, COLOR_FAINT);
  area512_sprite_push(filer->row, 0, filer->close_y);
}
