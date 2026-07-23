#if defined(PICORB_VM_MRUBYC)

#include "area512_hal.h"
#include "core/widget.h"

#include <string.h>

#define TEXT_VIEW_MAX_LINES 256
#define TEXT_VIEW_LINE_BUFFER_SIZE 256

typedef struct {
  int start;
  int length;
} WrappedLine;

static int
next_utf8_boundary(const char *text, int offset) {
  if (!text[offset])
    return offset;

  offset++;

  while (text[offset] && ((unsigned char)text[offset] & 0xC0) == 0x80)
    offset++;

  return offset;
}

static int
width_for_range(void *sprite, const char *text, int start, int length) {
  char buffer[TEXT_VIEW_LINE_BUFFER_SIZE];

  if (length >= (int)sizeof buffer)
    length = sizeof buffer - 1;

  memcpy(buffer, text + start, length);
  buffer[length] = 0;
  return area512_widget_text_width(sprite, buffer);
}

static int
append_wrapped_line(WrappedLine *lines, int line_count, int start, int length) {

  if (line_count >= TEXT_VIEW_MAX_LINES)
    return line_count;

  lines[line_count].start = start;
  lines[line_count].length = length;
  return line_count + 1;
}

static int
wrap_lines(
  void *sprite,
  const WidgetTextView *view,
  WrappedLine lines[TEXT_VIEW_MAX_LINES]
) {

  const char *text = view->text;
  int line_count = 0;
  int start = 0;

  while (text[start] && line_count < TEXT_VIEW_MAX_LINES) {
    int offset = start;
    int last_space = -1;

    while (text[offset] && text[offset] != '\n') {
      int next = next_utf8_boundary(text, offset);

      if (text[offset] == ' ')
        last_space = offset;

      if (width_for_range(sprite, text, start, next - start) > view->w) {
        int end = last_space > start ? last_space : offset;

        if (end == start)
          end = next;

        line_count = append_wrapped_line(lines, line_count, start, end - start);
        start = end;

        while (text[start] == ' ')
          start++;

        break;
      }

      offset = next;
    }

    if (start != offset && text[offset] && text[offset] != '\n')
      continue;

    line_count = append_wrapped_line(lines, line_count, start, offset - start);
    start = text[offset] == '\n' ? offset + 1 : offset;
  }

  if (line_count == 0 || (start > 0 && text[start - 1] == '\n'))
    line_count = append_wrapped_line(lines, line_count, start, 0);

  return line_count;
}

static int
visible_lines(const WidgetTextView *view) {
  int count = view->h / WIDGET_ROW_HEIGHT;
  return count > 0 ? count : 1;
}

static int
maximum_scroll(void *sprite, const WidgetTextView *view) {
  WrappedLine lines[TEXT_VIEW_MAX_LINES];
  int maximum = wrap_lines(sprite, view, lines) - visible_lines(view);
  return maximum > 0 ? maximum : 0;
}

void
area512_widget_text_view_init(WidgetTextView *view) {
  memset(view, 0, sizeof(*view));
  view->x = 0;
  view->y = area512_widget_body_top();
  view->w = area512_gfx_width();
  view->h = area512_widget_body_height();
}

void
area512_widget_text_view_set_text(WidgetTextView *view, const char *text) {
  int length = (int)strlen(text);

  if (length >= WIDGET_TEXT_VIEW_MAX)
    length = WIDGET_TEXT_VIEW_MAX - 1;

  while (length > 0 && ((unsigned char)text[length] & 0xC0) == 0x80)
    length--;

  memcpy(view->text, text, length);
  view->text[length] = 0;
  view->scroll = 0;
}

void
area512_widget_text_view_set_scroll(
  void *sprite,
  WidgetTextView *view,
  int scroll
) {

  if (scroll < 0)
    scroll = 0;

  int maximum = maximum_scroll(sprite, view);
  view->scroll = scroll > maximum ? maximum : scroll;
}

int
area512_widget_text_view_handle(
  void *sprite,
  WidgetTextView *view,
  const char *key
) {

  int delta = 0;

  if (strcmp(key, "UP") == 0 || strcmp(key, "k") == 0 || strcmp(key, ";") == 0)
    delta = -1;

  if (strcmp(key, "DOWN") == 0 || strcmp(key, "j") == 0 ||
      strcmp(key, ".") == 0)
    delta = 1;

  if (delta == 0)
    return 0;

  area512_widget_text_view_set_scroll(sprite, view, view->scroll + delta);
  return 1;
}

void
area512_widget_text_view_draw(void *sprite, const WidgetTextView *view) {
  WrappedLine lines[TEXT_VIEW_MAX_LINES];
  int line_count = wrap_lines(sprite, view, lines);
  int count = visible_lines(view);

  for (int row = 0; row < count; row++) {
    int line_index = view->scroll + row;

    if (line_index >= line_count)
      break;

    char text[TEXT_VIEW_LINE_BUFFER_SIZE];
    int length = lines[line_index].length;

    if (length >= (int)sizeof text)
      length = sizeof text - 1;

    memcpy(text, view->text + lines[line_index].start, length);
    text[length] = 0;
    area512_sprite_text(
      sprite,
      view->x,
      view->y + row * WIDGET_ROW_HEIGHT,
      text,
      WIDGET_COLOR_DIM
    );
  }

  area512_widget_draw_scrollbar(
    sprite,
    view->x + view->w - 2,
    view->y,
    view->h,
    view->scroll,
    count,
    line_count
  );
}

#endif
