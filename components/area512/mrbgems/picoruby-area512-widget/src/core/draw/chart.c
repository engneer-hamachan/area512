#if defined(PICORB_VM_MRUBYC)

#include "area512_hal.h"
#include "core/widget.h"

static int
chart_maximum(const int *values, int value_count, int maximum) {
  if (maximum > 0)
    return maximum;

  maximum = 0;

  for (int i = 0; i < value_count; i++) {
    if (values[i] > maximum)
      maximum = values[i];
  }

  return maximum;
}

static int
scaled_y(int y, int h, int value, int maximum) {
  if (value < 0)
    value = 0;

  if (value > maximum)
    value = maximum;

  return y + h - 1 - (h - 1) * value / maximum;
}

void
area512_widget_draw_bar_chart(
  void *sprite,
  int x,
  int y,
  int w,
  int h,
  const int *values,
  int value_count,
  int maximum
) {

  maximum = chart_maximum(values, value_count, maximum);

  if (maximum <= 0 || value_count <= 0 || w <= 0 || h <= 0)
    return;

  area512_sprite_line(
    sprite,
    x,
    y + h - 1,
    x + w - 1,
    y + h - 1,
    WIDGET_COLOR_AMBER
  );

  int slot_width = w / value_count;
  int bar_width = slot_width - 2;

  if (bar_width < 2)
    bar_width = 2;

  for (int i = 0; i < value_count; i++) {
    int bar_y = scaled_y(y, h, values[i], maximum);

    area512_sprite_fill_rect(
      sprite,
      x + i * slot_width,
      bar_y,
      bar_width,
      y + h - bar_y,
      WIDGET_COLOR_GOLD
    );
  }
}

void
area512_widget_draw_line_chart(
  void *sprite,
  int x,
  int y,
  int w,
  int h,
  const int *values,
  int value_count,
  int maximum
) {

  maximum = chart_maximum(values, value_count, maximum);

  if (maximum <= 0 || value_count <= 1 || w <= 0 || h <= 0)
    return;

  area512_sprite_line(
    sprite,
    x,
    y + h - 1,
    x + w - 1,
    y + h - 1,
    WIDGET_COLOR_AMBER
  );

  int previous_x = x;
  int previous_y = scaled_y(y, h, values[0], maximum);

  area512_sprite_fill_circle(
    sprite,
    previous_x,
    previous_y,
    1,
    WIDGET_COLOR_GOLD
  );

  for (int i = 1; i < value_count; i++) {
    int point_x = x + i * (w - 1) / (value_count - 1);
    int point_y = scaled_y(y, h, values[i], maximum);

    area512_sprite_line(
      sprite,
      previous_x,
      previous_y,
      point_x,
      point_y,
      WIDGET_COLOR_GOLD
    );
    area512_sprite_fill_circle(sprite, point_x, point_y, 1, WIDGET_COLOR_GOLD);

    previous_x = point_x;
    previous_y = point_y;
  }
}

#endif
