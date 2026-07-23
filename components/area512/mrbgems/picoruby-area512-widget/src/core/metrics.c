#if defined(PICORB_VM_MRUBYC)

#include "area512_hal.h"
#include "core/widget.h"

#include <string.h>

int
area512_widget_body_top(void) {
  return WIDGET_HEADER_HEIGHT + 3;
}

int
area512_widget_body_bottom(void) {
  return area512_gfx_height() - 16;
}

int
area512_widget_body_height(void) {
  return area512_widget_body_bottom() - area512_widget_body_top() + 1;
}

int
area512_widget_vcenter_text_y(int frame_y, int frame_height) {
  return frame_y + (frame_height - WIDGET_FONT_HEIGHT) / 2;
}

int
area512_widget_text_width(void *sprite, const char *text) {
  return area512_sprite_text_width(sprite, text);
}

static int
previous_utf8_boundary(const char *text, int offset) {
  if (offset <= 0)
    return 0;

  offset--;

  while (offset > 0 && ((unsigned char)text[offset] & 0xC0) == 0x80)
    offset--;

  return offset;
}

void
area512_widget_clip(
  void *sprite,
  const char *text,
  int width,
  char *output,
  int output_size
) {

  if (output_size <= 0)
    return;

  int length = (int)strlen(text);

  if (length >= output_size)
    length = previous_utf8_boundary(text, output_size);

  memcpy(output, text, length);
  output[length] = 0;

  if (area512_sprite_text_width(sprite, output) <= width)
    return;

  while (length > 0) {
    length = previous_utf8_boundary(output, length);

    if (length + 1 >= output_size)
      continue;

    output[length] = '>';
    output[length + 1] = 0;

    if (area512_sprite_text_width(sprite, output) <= width)
      return;
  }

  output[0] = 0;
}

#endif
