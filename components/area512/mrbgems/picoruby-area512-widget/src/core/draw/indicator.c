#if defined(PICORB_VM_MRUBYC)

#include "area512_hal.h"
#include "core/widget.h"

static int
clamp(int value, int minimum, int maximum) {
  if (value < minimum)
    return minimum;

  if (value > maximum)
    return maximum;

  return value;
}

void
area512_widget_draw_gauge(
  void *sprite,
  int x,
  int y,
  int w,
  int value,
  int maximum
) {

  if (maximum <= 0 || w < 4)
    return;

  value = clamp(value, 0, maximum);
  area512_sprite_rect(sprite, x, y, w, 9, WIDGET_COLOR_AMBER);
  area512_sprite_fill_rect(
    sprite,
    x + 2,
    y + 2,
    (w - 4) * value / maximum,
    5,
    WIDGET_COLOR_GOLD
  );
}

void
area512_widget_draw_slider(
  void *sprite,
  int x,
  int y,
  int w,
  int value,
  int maximum,
  int focused
) {

  if (maximum <= 0 || w < 7)
    return;

  value = clamp(value, 0, maximum);
  int knob_x = x + (w - 7) * value / maximum;

  area512_sprite_line(sprite, x, y + 5, x + w - 1, y + 5, WIDGET_COLOR_AMBER);

  if (focused) {
    area512_sprite_fill_rect(sprite, knob_x, y, 7, 11, WIDGET_COLOR_GOLD);
  } else {
    area512_sprite_fill_rect(sprite, knob_x, y, 7, 11, WIDGET_COLOR_DIM);
  }
}

void
area512_widget_draw_scrollbar(
  void *sprite,
  int x,
  int y,
  int h,
  int top,
  int visible,
  int total
) {

  if (total <= visible || visible <= 0 || h < 3)
    return;

  top = clamp(top, 0, total - visible);
  int thumb_height = h * visible / total;

  if (thumb_height < 6)
    thumb_height = 6;

  if (thumb_height > h)
    thumb_height = h;

  int thumb_y = y + h * top / total;

  if (thumb_y + thumb_height > y + h)
    thumb_y = y + h - thumb_height;

  area512_sprite_fill_rect(sprite, x, y, 3, h, WIDGET_COLOR_DARK);
  area512_sprite_fill_rect(
    sprite,
    x,
    thumb_y,
    3,
    thumb_height,
    WIDGET_COLOR_AMBER
  );
}

void
area512_widget_draw_horizontal_scrollbar(
  void *sprite,
  int x,
  int y,
  int w,
  int left,
  int visible,
  int total
) {

  if (total <= visible || visible <= 0 || w < 3)
    return;

  left = clamp(left, 0, total - visible);
  int thumb_width = w * visible / total;

  if (thumb_width < 6)
    thumb_width = 6;

  if (thumb_width > w)
    thumb_width = w;

  int thumb_x = x + w * left / total;

  if (thumb_x + thumb_width > x + w)
    thumb_x = x + w - thumb_width;

  area512_sprite_fill_rect(sprite, x, y, w, 3, WIDGET_COLOR_DARK);
  area512_sprite_fill_rect(
    sprite,
    thumb_x,
    y,
    thumb_width,
    3,
    WIDGET_COLOR_AMBER
  );
}

void
area512_widget_draw_badge(void *sprite, int x, int y, const char *text) {
  int width = area512_widget_text_width(sprite, text) + 8;

  area512_sprite_fill_rect(sprite, x, y, width, 13, WIDGET_COLOR_AMBER);
  area512_sprite_text(
    sprite,
    x + 4,
    area512_widget_vcenter_text_y(y, 13),
    text,
    WIDGET_COLOR_BG
  );
}

void
area512_widget_draw_battery(void *sprite, int x, int y) {
  int percentage = area512_metrics_battery_percent();

  area512_sprite_rect(sprite, x, y, 18, 9, WIDGET_COLOR_AMBER);
  area512_sprite_fill_rect(sprite, x + 18, y + 2, 2, 5, WIDGET_COLOR_AMBER);

  if (percentage < 0) {
    area512_sprite_text(
      sprite,
      x + 5,
      area512_widget_vcenter_text_y(y, 9),
      "?",
      WIDGET_COLOR_DIM
    );
    return;
  }

  percentage = clamp(percentage, 0, 100);
  area512_sprite_fill_rect(
    sprite,
    x + 2,
    y + 2,
    14 * percentage / 100,
    5,
    WIDGET_COLOR_GOLD
  );
}

void
area512_widget_draw_busy(void *sprite, int x, int y, int frame) {
  static const char frames[] = {'|', '/', '-', '\\'};
  char text[] = {frames[(unsigned int)frame % 4], 0};

  area512_sprite_text(sprite, x, y, text, WIDGET_COLOR_AMBER);
}

void
area512_widget_draw_page_dots(void *sprite, int y, int count, int active) {

  if (count <= 0)
    return;

  int total_width = count * 10 - 4;
  int x = (area512_gfx_width() - total_width) / 2;

  for (int i = 0; i < count; i++) {
    if (i == active) {
      area512_sprite_fill_circle(sprite, x + i * 10, y, 2, WIDGET_COLOR_GOLD);
    } else {
      area512_sprite_circle(sprite, x + i * 10, y, 2, WIDGET_COLOR_DIM);
    }
  }
}

#endif
