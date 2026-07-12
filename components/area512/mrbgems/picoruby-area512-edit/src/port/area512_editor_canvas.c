#include "port/area512_editor_canvas.h"

#include "core/syntax/picoruby/highlight.h"
#include "core/text/utf8.h"

#include <stdint.h>
#include <string.h>

void
clear_editor_canvas_row(void *context) {
  Area512EditorCanvas *canvas = (Area512EditorCanvas *)context;

  area512_sprite_set_font_size(canvas->row_sprite, canvas->font_size);
  area512_sprite_fill(canvas->row_sprite, EDIT_BACKGROUND);
}

void
draw_editor_canvas_row_text(
  void *context,
  int column,
  const char *text,
  int byte_length,
  uint32_t foreground,
  uint32_t background,
  int inverse
) {

  Area512EditorCanvas *canvas = (Area512EditorCanvas *)context;

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

  int pixel_left = column * canvas->char_width;

  if (draw_background != EDIT_BACKGROUND)
    area512_sprite_fill_rect(
      canvas->row_sprite,
      pixel_left,
      0,
      vim_display_width(text, byte_length) * canvas->char_width,
      canvas->row_height,
      draw_background
    );

  char text_buffer[256];

  int copy_byte_length = (int)sizeof(text_buffer) - 1;
  if (byte_length < copy_byte_length)
    copy_byte_length = byte_length;

  memcpy(text_buffer, text, copy_byte_length);

  text_buffer[copy_byte_length] = '\0';

  area512_sprite_text(
    canvas->row_sprite,
    pixel_left,
    0,
    text_buffer,
    draw_foreground
  );
}

void
push_editor_canvas_row(void *context, int row_index) {
  Area512EditorCanvas *canvas = (Area512EditorCanvas *)context;
  area512_sprite_push(canvas->row_sprite, 0, row_index * canvas->row_height);
}

int
editor_canvas_font_width(int font_size) {
  int width = (font_size + 1) / 2;

  return width < 1 ? 1 : width;
}

int
editor_canvas_font_row_span(int font_size) {
  int font_height = area512_sprite_font_height(font_size);
  int row_span = (font_height + EDIT_ROW_HEIGHT - 1) / EDIT_ROW_HEIGHT;

  return row_span < 1 ? 1 : row_span;
}

int
editor_canvas_font_row_height(int font_size) {
  return editor_canvas_font_row_span(font_size) * EDIT_ROW_HEIGHT;
}

void
set_editor_canvas_font_size(void *context, int font_size) {
  Area512EditorCanvas *canvas = (Area512EditorCanvas *)context;

  canvas->font_size = font_size;
  canvas->char_width = editor_canvas_font_width(font_size);
  area512_sprite_set_font_size(canvas->row_sprite, font_size);
}

void
draw_editor_canvas_cursor(
  void *context,
  int column,
  int row_index,
  int visible
) {
  Area512EditorCanvas *canvas = (Area512EditorCanvas *)context;

  if (!visible || !canvas->cursor_sprite)
    return;

  area512_sprite_fill(canvas->cursor_sprite, EDIT_CURSOR_KEY);

  area512_sprite_fill_rect(
    canvas->cursor_sprite,
    0,
    canvas->row_height - 2,
    canvas->char_width,
    2,
    EDIT_FOREGROUND
  );

  area512_sprite_push_transparent(
    canvas->cursor_sprite,
    column * canvas->char_width,
    row_index * canvas->row_height,
    EDIT_CURSOR_KEY
  );
}

typedef struct {
  VimCanvas *canvas;
  int column;
} highlight_canvas_context;

static void
draw_highlight_segment(
  void *writer_context,
  const char *text,
  int byte_length,
  uint32_t color
) {

  highlight_canvas_context *context = (highlight_canvas_context *)writer_context;

  context->canvas->draw_row_text(
    context->canvas->context,
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
  VimCanvas *canvas,
  int column,
  const char *segment,
  int segment_byte_length
) {

  if (segment_byte_length <= 0)
    return;

  highlight_canvas_context writer_context = {canvas, column};

  editor_highlight_context_t highlight_context;

  editor_highlight_init(
    &highlight_context,
    (const uint8_t *)segment,
    segment_byte_length,
    draw_highlight_segment,
    &writer_context
  );

  editor_highlight_run(&highlight_context);
}
