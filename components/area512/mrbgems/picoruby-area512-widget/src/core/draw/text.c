#if defined(PICORB_VM_MRUBYC)

#include "area512_hal.h"
#include "core/widget.h"

#include <string.h>

#define WRAP_LINE_BUFFER_SIZE 256

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
text_width_for_bytes(void *sprite, const char *text, int length) {
  char buffer[WRAP_LINE_BUFFER_SIZE];

  if (length >= (int)sizeof buffer)
    length = sizeof buffer - 1;

  memcpy(buffer, text, length);
  buffer[length] = 0;
  return area512_widget_text_width(sprite, buffer);
}

static int
wrapped_line_length(void *sprite, const char *text, int width) {
  int offset = 0;
  int last_space = -1;

  while (text[offset] && text[offset] != '\n') {
    int next = next_utf8_boundary(text, offset);

    if (text[offset] == ' ')
      last_space = offset;

    if (text_width_for_bytes(sprite, text, next) > width)
      return last_space > 0 ? last_space : offset;

    offset = next;
  }

  return offset;
}

static const char *
skip_line_separator(const char *text) {
  if (*text == '\n')
    return text + 1;

  while (*text == ' ')
    text++;

  return text;
}

void
area512_widget_draw_text_center(
  void *sprite,
  int y,
  const char *text,
  uint32_t color
) {

  int x = (area512_gfx_width() - area512_widget_text_width(sprite, text)) / 2;
  area512_sprite_text(sprite, x, y, text, color);
}

void
area512_widget_draw_text_right(
  void *sprite,
  int right_x,
  int y,
  const char *text,
  uint32_t color
) {

  int x = right_x - area512_widget_text_width(sprite, text);
  area512_sprite_text(sprite, x, y, text, color);
}

void
area512_widget_draw_center_lines(
  void *sprite,
  const WidgetColoredLine *lines,
  int line_count
) {

  int y = (area512_gfx_height() - line_count * WIDGET_ROW_HEIGHT) / 2;

  for (int i = 0; i < line_count; i++) {
    int line_y = area512_widget_vcenter_text_y(
      y + i * WIDGET_ROW_HEIGHT,
      WIDGET_ROW_HEIGHT
    );

    area512_widget_draw_text_center(
      sprite,
      line_y,
      lines[i].text,
      lines[i].color
    );
  }
}

int
area512_widget_draw_wrap_text(
  void *sprite,
  int x,
  int y,
  int w,
  int h,
  const char *text,
  uint32_t color
) {

  int maximum_lines = h / WIDGET_ROW_HEIGHT;
  int line = 0;

  while (*text && line < maximum_lines) {
    char buffer[WRAP_LINE_BUFFER_SIZE];
    int length = wrapped_line_length(sprite, text, w);

    if (length == 0 && *text != '\n')
      length = next_utf8_boundary(text, 0);

    if (length >= (int)sizeof buffer)
      length = sizeof buffer - 1;

    memcpy(buffer, text, length);
    buffer[length] = 0;
    area512_sprite_text(sprite, x, y + line * WIDGET_ROW_HEIGHT, buffer, color);

    text = skip_line_separator(text + length);
    line++;
  }

  return line;
}

int
area512_widget_draw_marquee(
  void *sprite,
  int x,
  int y,
  int w,
  const char *text,
  int offset
) {

  int text_width = area512_widget_text_width(sprite, text);
  int start = 0;
  int start_width = 0;

  if (offset < 0)
    offset = 0;

  while (text[start]) {
    int next = next_utf8_boundary(text, start);
    int next_width = text_width_for_bytes(sprite, text, next);

    if (next_width > offset)
      break;

    start = next;
    start_width = next_width;
  }

  char visible[WRAP_LINE_BUFFER_SIZE];
  int length = 0;

  while (text[start + length]) {
    int next = next_utf8_boundary(text + start, length);

    if (text_width_for_bytes(sprite, text + start, next) > w)
      break;

    length = next;
  }

  if (length >= (int)sizeof visible)
    length = sizeof visible - 1;

  memcpy(visible, text + start, length);
  visible[length] = 0;
  area512_sprite_text(
    sprite,
    x - (offset - start_width),
    y,
    visible,
    WIDGET_COLOR_DIM
  );

  return text_width;
}

void
area512_widget_draw_big_text(
  void *sprite,
  int x,
  int y,
  const char *text,
  uint32_t color
) {

  area512_sprite_set_font_size(sprite, 24);
  area512_sprite_text(sprite, x, y, text, color);
  area512_sprite_set_font_size(sprite, 12);
}

#endif
