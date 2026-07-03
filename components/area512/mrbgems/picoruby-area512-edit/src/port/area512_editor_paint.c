#include "port/area512_editor_paint.h"

#include "core/syntax/picoruby/highlight.h"
#include "core/text/utf8.h"

#include <stdint.h>
#include <string.h>

void
clear_editor_paint_row(void *paint_context) {
  editor_paint *paint = (editor_paint *)paint_context;
  area512_sprite_fill(paint->row_sprite, EDIT_BACKGROUND);
}

void
draw_editor_paint_row_text(
  void *paint_context,
  int column,
  const char *text,
  int byte_length,
  uint32_t foreground,
  uint32_t background,
  int inverse
) {

  editor_paint *paint = (editor_paint *)paint_context;

  if (byte_length <= 0)
    return;

  uint32_t draw_foreground = EDIT_FOREGROUND;
  if (foreground)
    draw_foreground = foreground;

  uint32_t draw_background = EDIT_BACKGROUND;
  if (background)
    draw_background = background;

  if (inverse) {
    uint32_t previous_foreground = draw_foreground;
    draw_foreground = draw_background;
    draw_background = previous_foreground;
  }

  int pixel_left = column * paint->char_width;

  if (draw_background != EDIT_BACKGROUND)
    area512_sprite_fill_rect(
      paint->row_sprite,
      pixel_left,
      0,
      vim_display_width(text, byte_length) * paint->char_width,
      paint->row_height,
      draw_background
    );

  char text_buffer[256];

  int copy_byte_length = (int)sizeof(text_buffer) - 1;
  if (byte_length < copy_byte_length)
    copy_byte_length = byte_length;

  memcpy(text_buffer, text, copy_byte_length);

  text_buffer[copy_byte_length] = '\0';

  area512_sprite_text(
    paint->row_sprite,
    pixel_left,
    0,
    text_buffer,
    draw_foreground
  );
}

void
push_editor_paint_row(void *paint_context, int row_index) {
  editor_paint *paint = (editor_paint *)paint_context;
  area512_sprite_push(paint->row_sprite, 0, row_index * paint->row_height);
}

void
draw_editor_paint_cursor(
  void *paint_context,
  int column,
  int row_index,
  int visible
) {
  editor_paint *paint = (editor_paint *)paint_context;

  if (!visible || !paint->cursor_sprite)
    return;

  area512_sprite_fill(paint->cursor_sprite, EDIT_CURSOR_KEY);

  area512_sprite_fill_rect(
    paint->cursor_sprite,
    0,
    paint->row_height - 2,
    paint->char_width,
    2,
    EDIT_FOREGROUND
  );

  area512_sprite_push_transparent(
    paint->cursor_sprite,
    column * paint->char_width,
    row_index * paint->row_height,
    EDIT_CURSOR_KEY
  );
}

typedef struct {
  VimPaint *paint;
  int column;
} highlight_paint_context;

static void
paint_highlight_segment(
  void *writer_context,
  const char *text,
  int byte_length,
  uint32_t color
) {

  highlight_paint_context *context = (highlight_paint_context *)writer_context;

  context->paint->draw_row_text(
    context->paint->paint_context,
    context->column,
    text,
    byte_length,
    color,
    0,
    0
  );

  context->column += vim_display_width(text, byte_length);
}

void
highlight_edit_segment(
  VimPaint *paint,
  int column,
  const char *segment,
  int segment_byte_length
) {

  if (segment_byte_length <= 0)
    return;

  highlight_paint_context writer_context = {paint, column};

  editor_highlight_context_t highlight_context;

  editor_highlight_init(
    &highlight_context,
    (const uint8_t *)segment,
    segment_byte_length,
    paint_highlight_segment,
    &writer_context
  );

  editor_highlight_run(&highlight_context);
}
