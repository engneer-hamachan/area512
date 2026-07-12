#if defined(PICORB_VM_MRUBYC)

#include "area512_hal.h"
#include "core/widget.h"

#include <string.h>

static void
copy_bounded(char *destination, int destination_size, const char *source) {
  int length = (int)strlen(source);

  if (length >= destination_size)
    length = destination_size - 1;

  memcpy(destination, source, length);
  destination[length] = 0;
}

static int
visible_rows(const WidgetList *list) {
  int rows = list->h / WIDGET_ROW_HEIGHT;
  return rows > 0 ? rows : 1;
}

static void
clamp_list_position(WidgetList *list) {
  if (list->count == 0) {
    list->index = 0;
    list->top = 0;
    return;
  }

  if (list->index < 0)
    list->index = 0;

  if (list->index >= list->count)
    list->index = list->count - 1;

  int rows = visible_rows(list);

  if (list->index < list->top)
    list->top = list->index;

  if (list->index >= list->top + rows)
    list->top = list->index - rows + 1;

  int maximum_top = list->count - rows;

  if (maximum_top < 0)
    maximum_top = 0;

  if (list->top > maximum_top)
    list->top = maximum_top;

  if (list->top < 0)
    list->top = 0;
}

void
area512_widget_list_init(WidgetList *list) {
  memset(list, 0, sizeof(*list));
  list->x = 0;
  list->y = area512_widget_body_top();
  list->w = area512_gfx_width();
  list->h = area512_widget_body_height();
  copy_bounded(list->empty_text, sizeof list->empty_text, "No items");
}

void
area512_widget_list_clear(WidgetList *list) {
  list->index = 0;
  list->top = 0;
  list->count = 0;
  memset(list->marks, 0, sizeof list->marks);
}

void
area512_widget_list_add(WidgetList *list, const char *text, const char *tag) {
  if (list->count >= WIDGET_LIST_MAX_ROWS)
    return;

  int index = list->count;

  copy_bounded(list->texts[index], WIDGET_LIST_TEXT_MAX, text);
  copy_bounded(list->tags[index], WIDGET_LIST_TAG_MAX, tag);
  list->marks[index] = 0;
  list->count++;
}

void
area512_widget_list_set_index(WidgetList *list, int index) {
  list->index = index;
  clamp_list_position(list);
}

int
area512_widget_list_handle(WidgetList *list, const char *key) {
  int delta = 0;

  if (strcmp(key, "UP") == 0 || strcmp(key, "k") == 0 || strcmp(key, ";") == 0)
    delta = -1;

  if (strcmp(key, "DOWN") == 0 || strcmp(key, "j") == 0 ||
      strcmp(key, ".") == 0)
    delta = 1;

  if (delta == 0)
    return 0;

  area512_widget_list_set_index(list, list->index + delta);
  return 1;
}

static int
draw_mark(void *sprite, int x, int y, int marked, uint32_t color) {
  area512_sprite_text(sprite, x, y, marked ? "[x]" : "[ ]", color);
  return x + 4 * WIDGET_CHAR_WIDTH;
}

static int
draw_tag(void *sprite, int x, int y, const char *tag) {
  if (!tag[0])
    return x;

  area512_sprite_text(sprite, x, y, tag, WIDGET_COLOR_AMBER);
  return x + area512_widget_text_width(sprite, tag) + WIDGET_CHAR_WIDTH;
}

static void
draw_row(void *sprite, const WidgetList *list, int index, int y) {
  int selected = (index == list->index);
  int content_width = list->w - 6;
  int x = list->x + 3;
  char clipped[WIDGET_LIST_TEXT_MAX];

  if (selected) {
    area512_sprite_fill_rect(
      sprite,
      list->x,
      y,
      list->w - 4,
      WIDGET_ROW_HEIGHT - 1,
      WIDGET_COLOR_DARK
    );
    area512_sprite_rect(
      sprite,
      list->x,
      y,
      list->w - 4,
      WIDGET_ROW_HEIGHT - 1,
      WIDGET_COLOR_GOLD
    );
  }

  uint32_t color = selected ? WIDGET_COLOR_GOLD : WIDGET_COLOR_DIM;
  int text_y = area512_widget_vcenter_text_y(y, WIDGET_ROW_HEIGHT);

  if (list->show_marks)
    x = draw_mark(sprite, x, text_y, list->marks[index], color);

  x = draw_tag(sprite, x, text_y, list->tags[index]);
  content_width -= x - list->x;
  area512_widget_clip(
    sprite,
    list->texts[index],
    content_width,
    clipped,
    sizeof clipped
  );
  area512_sprite_text(sprite, x, text_y, clipped, color);
}

void
area512_widget_list_draw(void *sprite, const WidgetList *list) {
  if (list->count == 0) {
    int text_width = area512_widget_text_width(sprite, list->empty_text);
    int x = list->x + (list->w - text_width) / 2;
    int y = area512_widget_vcenter_text_y(list->y, list->h);
    area512_sprite_text(sprite, x, y, list->empty_text, WIDGET_COLOR_DIM);
    return;
  }

  int rows = visible_rows(list);

  for (int row = 0; row < rows; row++) {
    int index = list->top + row;

    if (index >= list->count)
      break;

    draw_row(sprite, list, index, list->y + row * WIDGET_ROW_HEIGHT);
  }

  area512_widget_draw_scrollbar(
    sprite,
    list->x + list->w - 2,
    list->y,
    list->h,
    list->top,
    rows,
    list->count
  );
}

#endif
