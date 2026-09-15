#include "core/draw/draw.h"

#include <stdio.h>

#define BRAND_TEXT "AREA 512"
#define BRAND_WIDTH 99
#define BRAND_X ((SCREEN_WIDTH - BRAND_WIDTH) / 2)

#define PATH_X (FRAME_INSET + FRAME_RADIUS + 4)
#define PATH_COLUMNS ((BRAND_X - PATH_X - 4) / FILER_CHAR_WIDTH)

void
draw_frame(Filer *filer) {
  int height = CLOSE_Y + FRAME_BOTTOM_HEIGHT - FRAME_TOP;

  if (filer->has_background_image)
    area512_screen_read_sprite(filer->screen, filer->row, 0, FRAME_TOP);

  for (int inset = 0; inset < 2; inset++) {
    area512_sprite_round_rect(
      filer->screen,
      FRAME_INSET + inset,
      FRAME_TOP + inset,
      SCREEN_WIDTH - FRAME_INSET * 2 - inset * 2,
      height - inset * 2,
      FRAME_RADIUS - inset,
      area512_theme_border_color()
    );
  }
}

static void
clear_frame_rect(Filer *filer, int x, int y, int width, int height) {
  if (filer->has_background_image) {
    area512_screen_draw_sprite_clipped(
      filer->screen,
      filer->row,
      0,
      FRAME_TOP,
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
edge_text(
  Filer *filer,
  int x,
  int y,
  int height,
  const char *text,
  uint32_t color
) {

  int width = area512_sprite_text_width(filer->screen, text);

  clear_frame_rect(filer, x - 2, y, width + 4, height);
  area512_sprite_text(filer->screen, x, y, text, color);
}

void
draw_header(Filer *filer) {
  area512_sprite_set_title_font(filer->screen);

  clear_frame_rect(
    filer,
    BRAND_X - 2,
    FRAME_INSET,
    BRAND_WIDTH + 4,
    HEADER_HEIGHT - FRAME_INSET
  );

  area512_sprite_text(
    filer->screen,
    BRAND_X,
    FRAME_INSET,
    BRAND_TEXT,
    area512_theme_emphasis_color()
  );

  area512_sprite_set_font_size(filer->screen, FILER_FONT_SIZE);

  char count[24];

  snprintf(
    count,
    sizeof count,
    "%d/%d",
    filer->index + 1,
    filer->count
  );

  int count_x =
    SCREEN_WIDTH -
    FRAME_INSET -
    FRAME_RADIUS -
    4 -
    area512_sprite_text_width(filer->screen, count);

  int text_y = FRAME_TOP - FILER_FONT_SIZE / 2;

  edge_text(
    filer,
    count_x,
    text_y,
    FILER_FONT_SIZE,
    count,
    area512_theme_emphasis_color()
  );

  char fitted_path[LINE_MAX];

  fit_string(
    fitted_path,
    sizeof fitted_path,
    filer->current_directory,
    PATH_COLUMNS
  );

  edge_text(
    filer,
    PATH_X,
    text_y,
    FILER_FONT_SIZE,
    fitted_path,
    area512_theme_text_color()
  );
}
