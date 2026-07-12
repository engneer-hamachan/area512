#if defined(PICORB_VM_MRUBYC)

#include "area512_hal.h"
#include "core/widget.h"

#define PANEL_TEXT_BUFFER_SIZE 128

void
area512_widget_draw_panel(void *sprite, int x, int y, int w, int h) {
  if (w <= 0 || h <= 0)
    return;

  area512_sprite_fill_rect(sprite, x, y, w, h, WIDGET_COLOR_DARK);
  area512_sprite_rect(sprite, x, y, w, h, WIDGET_COLOR_AMBER);
}

void
area512_widget_draw_titled_panel(
  void *sprite,
  int x,
  int y,
  int w,
  int h,
  const char *title
) {

  char clipped[PANEL_TEXT_BUFFER_SIZE];

  area512_widget_draw_panel(sprite, x, y, w, h);
  area512_widget_clip(sprite, title, w - 8, clipped, sizeof clipped);
  area512_sprite_text(
    sprite,
    x + 4,
    area512_widget_vcenter_text_y(y, 15),
    clipped,
    WIDGET_COLOR_GOLD
  );
  area512_sprite_line(sprite, x, y + 14, x + w - 1, y + 14, WIDGET_COLOR_AMBER);
}

void
area512_widget_draw_toast(void *sprite, const char *message) {
  int width = area512_widget_text_width(sprite, message) + 16;

  if (width > area512_gfx_width() - 16)
    width = area512_gfx_width() - 16;

  int x = (area512_gfx_width() - width) / 2;
  int y = area512_gfx_height() - 34;
  char clipped[PANEL_TEXT_BUFFER_SIZE];

  area512_widget_draw_panel(sprite, x, y, width, 18);
  area512_widget_clip(sprite, message, width - 16, clipped, sizeof clipped);
  area512_sprite_text(
    sprite,
    x + 8,
    area512_widget_vcenter_text_y(y, 18),
    clipped,
    WIDGET_COLOR_GOLD
  );
}

void
area512_widget_draw_splash(
  void *sprite,
  const char *title,
  const char *subtitle
) {

  int center_y = area512_gfx_height() / 2;

  area512_sprite_fill(sprite, WIDGET_COLOR_BG);
  area512_sprite_set_font_size(sprite, 24);
  area512_widget_draw_text_center(
    sprite,
    center_y - 18,
    title,
    WIDGET_COLOR_GOLD
  );
  area512_sprite_set_font_size(sprite, 12);

  if (subtitle[0]) {
    area512_widget_draw_text_center(
      sprite,
      center_y + 12,
      subtitle,
      WIDGET_COLOR_DIM
    );
  }
}

#endif
