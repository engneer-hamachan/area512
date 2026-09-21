#include "core/draw/draw.h"

#include <stdio.h>
#include <string.h>

void
draw_frame(Filer *filer) {
  int outer_top = HALF_ROW_HEIGHT - 1;
  int inner_top = HALF_ROW_HEIGHT + 1;
  int outer_bottom = filer->close_y + 2;
  int inner_bottom = filer->close_y;
  int right = filer->width - 1;

  if (filer->has_background_image)
    area512_screen_read_sprite(filer->screen, filer->row, 0, 0);

  area512_sprite_line(
    filer->screen,
    0,
    outer_top,
    right,
    outer_top,
    area512_theme_border_color()
  );

  area512_sprite_line(
    filer->screen,
    0,
    outer_bottom,
    right,
    outer_bottom,
    area512_theme_border_color()
  );

  area512_sprite_line(
    filer->screen,
    0,
    outer_top,
    0,
    outer_bottom,
    area512_theme_border_color()
  );

  area512_sprite_line(
    filer->screen,
    right,
    outer_top,
    right,
    outer_bottom,
    area512_theme_border_color()
  );

  area512_sprite_line(
    filer->screen,
    2,
    inner_top,
    right - 2,
    inner_top,
    area512_theme_border_color()
  );

  area512_sprite_line(
    filer->screen,
    2,
    inner_bottom,
    right - 2,
    inner_bottom,
    area512_theme_border_color()
  );

  area512_sprite_line(
    filer->screen,
    2,
    inner_top,
    2,
    inner_bottom,
    area512_theme_border_color()
  );

  area512_sprite_line(
    filer->screen,
    right - 2,
    inner_top,
    right - 2,
    inner_bottom,
    area512_theme_border_color()
  );
}

static void
clear_frame_rect(Filer *filer, int x, int y, int width, int height) {
  if (filer->has_background_image) {
    area512_screen_draw_sprite_clipped(
      filer->screen,
      filer->row,
      0,
      0,
      x,
      y,
      width,
      height
    );
    return;
  }

  area512_sprite_fill_rect(
    filer->screen,
    x,
    y,
    width,
    height,
    area512_theme_background_color()
  );
}

static void
edge_text(Filer *filer, int x, const char *text, uint32_t color) {
  int width = (int)strlen(text) * FILER_CHAR_WIDTH;

  clear_frame_rect(filer, x - 1, 0, width + 2, ROW_HEIGHT);

  area512_sprite_text(filer->screen, x, 0, text, color);
}

void
draw_header(Filer *filer) {
  const char *brand = " AREA 512 ";
  int brand_x = (filer->width - (int)strlen(brand) * FILER_CHAR_WIDTH) / 2;

  edge_text(filer, brand_x, brand, area512_theme_emphasis_color());

  char count[24];
  snprintf(count, sizeof count, " %d/%d ", filer->index + 1, filer->count);

  edge_text(
    filer,
    filer->width - 6 - (int)strlen(count) * FILER_CHAR_WIDTH,
    count,
    area512_theme_emphasis_color()
  );

  int available = (brand_x - 6) / FILER_CHAR_WIDTH;

  if (available > 1) {
    char path[CURRENT_DIRECTORY_MAX + 2];
    char fitted_path[LINE_MAX];

    snprintf(path, sizeof path, " %s ", filer->current_directory);

    fit_string(fitted_path, sizeof fitted_path, path, available);

    edge_text(filer, 4, fitted_path, area512_theme_text_color());
  }
}
