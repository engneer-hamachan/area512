#if defined(PICORB_VM_MRUBYC)

#include "area512_hal.h"
#include "core/widget.h"

#include <stdio.h>

static uint32_t
focus_color(int focused) {
  return focused ? WIDGET_COLOR_GOLD : WIDGET_COLOR_DIM;
}

void
area512_widget_draw_button(
  void *sprite,
  int x,
  int y,
  int w,
  const char *label,
  int focused
) {

  if (focused)
    area512_sprite_fill_rect(sprite, x, y, w, 15, WIDGET_COLOR_DARK);

  area512_sprite_rect(
    sprite,
    x,
    y,
    w,
    15,
    focused ? WIDGET_COLOR_GOLD : WIDGET_COLOR_AMBER
  );

  int text_x = x + (w - area512_widget_text_width(sprite, label)) / 2;
  area512_sprite_text(
    sprite,
    text_x,
    area512_widget_vcenter_text_y(y, 15),
    label,
    focus_color(focused)
  );
}

void
area512_widget_draw_checkbox(
  void *sprite,
  int x,
  int y,
  const char *label,
  int checked,
  int focused
) {

  area512_sprite_rect(
    sprite,
    x,
    y + 1,
    9,
    9,
    focused ? WIDGET_COLOR_GOLD : WIDGET_COLOR_AMBER
  );

  if (checked) {
    area512_sprite_fill_rect(sprite, x + 2, y + 3, 5, 5, WIDGET_COLOR_GOLD);
  }

  area512_sprite_text(sprite, x + 13, y, label, focus_color(focused));
}

void
area512_widget_draw_radio(
  void *sprite,
  int x,
  int y,
  const char *label,
  int selected,
  int focused
) {

  area512_sprite_circle(
    sprite,
    x + 4,
    y + 5,
    4,
    focused ? WIDGET_COLOR_GOLD : WIDGET_COLOR_AMBER
  );

  if (selected) {
    area512_sprite_fill_circle(sprite, x + 4, y + 5, 2, WIDGET_COLOR_GOLD);
  }

  area512_sprite_text(sprite, x + 13, y, label, focus_color(focused));
}

void
area512_widget_draw_toggle(void *sprite, int x, int y, int on, int focused) {

  area512_sprite_rect(
    sprite,
    x,
    y,
    24,
    11,
    focused ? WIDGET_COLOR_GOLD : WIDGET_COLOR_AMBER
  );
  area512_sprite_fill_rect(
    sprite,
    x + (on ? 13 : 2),
    y + 2,
    9,
    7,
    on ? WIDGET_COLOR_GOLD : WIDGET_COLOR_DIM
  );
}

void
area512_widget_draw_spinner(
  void *sprite,
  int x,
  int y,
  int w,
  const char *text,
  int focused
) {

  uint32_t arrow_color = focused ? WIDGET_COLOR_GOLD : WIDGET_COLOR_AMBER;
  int text_x = x + (w - area512_widget_text_width(sprite, text)) / 2;

  area512_sprite_text(sprite, x, y, "<", arrow_color);
  area512_sprite_text(sprite, x + w - 6, y, ">", arrow_color);
  area512_sprite_text(sprite, text_x, y, text, focus_color(focused));
}

#endif
