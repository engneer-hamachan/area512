#if defined(PICORB_VM_MRUBYC)

#include "area512_hal.h"
#include "core/widget.h"

#define TABLE_TEXT_BUFFER_SIZE 128

void
area512_widget_draw_cell(
  void *sprite,
  int x,
  int y,
  int w,
  int h,
  const char *text,
  int selected
) {

  char clipped[TABLE_TEXT_BUFFER_SIZE];
  uint32_t border = selected ? WIDGET_COLOR_GOLD : WIDGET_COLOR_AMBER;
  uint32_t text_color = selected ? WIDGET_COLOR_GOLD : WIDGET_COLOR_DIM;

  if (selected)
    area512_sprite_fill_rect(sprite, x, y, w, h, WIDGET_COLOR_DARK);

  area512_sprite_rect(sprite, x, y, w, h, border);
  area512_widget_clip(sprite, text, w - 6, clipped, sizeof clipped);
  area512_sprite_text(
    sprite,
    x + 3,
    area512_widget_vcenter_text_y(y, h),
    clipped,
    text_color
  );
}

void
area512_widget_draw_table_header(
  void *sprite,
  int x,
  int y,
  const int *widths,
  const char *const *labels,
  int column_count
) {

  for (int i = 0; i < column_count; i++) {
    area512_widget_draw_cell(
      sprite,
      x,
      y,
      widths[i],
      WIDGET_ROW_HEIGHT,
      labels[i],
      0
    );
    x += widths[i];
  }
}

void
area512_widget_draw_table_row(
  void *sprite,
  int x,
  int y,
  const int *widths,
  const char *const *values,
  int column_count,
  int selected
) {

  for (int i = 0; i < column_count; i++) {
    area512_widget_draw_cell(
      sprite,
      x,
      y,
      widths[i],
      WIDGET_ROW_HEIGHT,
      values[i],
      selected
    );
    x += widths[i];
  }
}

void
area512_widget_draw_field(
  void *sprite,
  int x,
  int y,
  int w,
  const char *label,
  const char *value,
  int focused
) {

  int text_y = area512_widget_vcenter_text_y(y, 15);

  if (focused) {
    area512_sprite_fill_rect(sprite, x, y, w, 15, WIDGET_COLOR_DARK);
    area512_sprite_rect(sprite, x, y, w, 15, WIDGET_COLOR_GOLD);
  }

  area512_sprite_text(
    sprite,
    x + 3,
    text_y,
    label,
    focused ? WIDGET_COLOR_GOLD : WIDGET_COLOR_DIM
  );
  area512_widget_draw_text_right(
    sprite,
    x + w - 3,
    text_y,
    value,
    WIDGET_COLOR_GOLD
  );
}

#endif
