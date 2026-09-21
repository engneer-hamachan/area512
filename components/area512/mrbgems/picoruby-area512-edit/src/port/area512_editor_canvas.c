#include "port/area512_editor_canvas.h"

#include "core/syntax/picoruby/highlight.h"
#include "core/syntax/python/highlight.h"
#include "core/text/utf8.h"

#include <stdint.h>
#include <string.h>

void
clear_editor_canvas_row(void *context) {
  Area512EditorCanvas *canvas = (Area512EditorCanvas *)context;

  area512_sprite_set_font_size(canvas->row_sprite, canvas->font_size);
  area512_sprite_fill(canvas->row_sprite, area512_theme_background_color());
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

  if (inverse) {
    uint32_t previous_foreground = foreground;
    foreground = background;
    background = previous_foreground;
  }

  int pixel_left = column * canvas->char_width;

  area512_sprite_fill_rect(
    canvas->row_sprite,
    pixel_left,
    0,
    vim_display_width(text, byte_length) * canvas->char_width,
    canvas->row_height,
    background
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
    foreground
  );
}

static int
compute_row_pixel_top(Area512EditorCanvas *canvas, int row_index) {
  int last_row_index = area512_gfx_height() / canvas->row_height - 1;

  if (row_index == last_row_index)
    return area512_gfx_height() - canvas->row_height;

  return row_index * canvas->row_height;
}

void
push_editor_canvas_row(void *context, int row_index) {
  Area512EditorCanvas *canvas = (Area512EditorCanvas *)context;
  area512_sprite_push(
    canvas->row_sprite,
    0,
    compute_row_pixel_top(canvas, row_index)
  );
}

void
fill_editor_canvas_row_span(
  void *context,
  int column,
  int column_count,
  uint32_t color
) {
  Area512EditorCanvas *canvas = (Area512EditorCanvas *)context;

  if (column_count <= 0)
    return;

  area512_sprite_fill_rect(
    canvas->row_sprite,
    column * canvas->char_width,
    0,
    column_count * canvas->char_width,
    canvas->row_height,
    color
  );
}

void
draw_editor_canvas_row_underline(
  void *context,
  int column,
  int column_count,
  uint32_t color
) {
  Area512EditorCanvas *canvas = (Area512EditorCanvas *)context;

  if (column_count <= 0)
    return;

  int pixel_left = column * canvas->char_width;
  int pixel_right = (column + column_count) * canvas->char_width - 1;
  int pixel_top = canvas->row_height - 1;

  area512_sprite_line(
    canvas->row_sprite,
    pixel_left,
    pixel_top,
    pixel_right,
    pixel_top,
    color
  );
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
  int visible,
  VimMode mode
) {
  Area512EditorCanvas *canvas = (Area512EditorCanvas *)context;

  if (!visible || !canvas->cursor_sprite)
    return;

  area512_sprite_fill(canvas->cursor_sprite, EDIT_CURSOR_KEY);

  if (mode == VIM_MODE_INSERT)
    area512_sprite_fill_rect(
      canvas->cursor_sprite,
      0,
      1,
      2,
      canvas->row_height - 2,
      area512_theme_selected_color()
    );
  else
    area512_sprite_fill_rect(
      canvas->cursor_sprite,
      0,
      canvas->row_height - 2,
      canvas->char_width,
      2,
      area512_theme_selected_color()
    );

  area512_sprite_push_transparent(
    canvas->cursor_sprite,
    column * canvas->char_width,
    compute_row_pixel_top(canvas, row_index),
    EDIT_CURSOR_KEY
  );
}

typedef struct {
  VimCanvas *canvas;
  int column;
  int next_segment_byte_offset;
  int visible_byte_begin;
  int visible_byte_end;
} highlight_canvas_context;

static void
draw_highlight_segment(
  void *writer_context,
  const char *text,
  int byte_length,
  uint32_t color
) {

  highlight_canvas_context *context =
    (highlight_canvas_context *)writer_context;

  int segment_byte_begin = context->next_segment_byte_offset;
  int segment_byte_end = segment_byte_begin + byte_length;

  context->next_segment_byte_offset = segment_byte_end;

  int drawn_byte_begin = segment_byte_begin;
  if (drawn_byte_begin < context->visible_byte_begin)
    drawn_byte_begin = context->visible_byte_begin;

  int drawn_byte_end = segment_byte_end;
  if (drawn_byte_end > context->visible_byte_end)
    drawn_byte_end = context->visible_byte_end;

  if (drawn_byte_end <= drawn_byte_begin)
    return;

  const char *drawn_text = text + (drawn_byte_begin - segment_byte_begin);
  int drawn_byte_length = drawn_byte_end - drawn_byte_begin;

  context->canvas->draw_row_text(
    context->canvas->context,
    context->column,
    drawn_text,
    drawn_byte_length,
    color,
    area512_theme_background_color(),
    0
  );

  context->column += vim_display_width(drawn_text, drawn_byte_length);
}

static void
run_ruby_highlight(
  highlight_canvas_context *writer_context,
  const char *text,
  int text_byte_length
) {

  editor_highlight_context_t highlight_context;

  editor_highlight_init(
    &highlight_context,
    (const uint8_t *)text,
    text_byte_length,
    draw_highlight_segment,
    writer_context
  );

  editor_highlight_run(&highlight_context);
}

void
highlight_visible_row_text(
  VimCanvas *canvas,
  VimSyntax syntax,
  int column,
  const char *text,
  int text_byte_length,
  int visible_byte_begin,
  int visible_byte_end
) {

  if (visible_byte_end <= visible_byte_begin)
    return;

  highlight_canvas_context writer_context = {
    canvas,
    column,
    0,
    visible_byte_begin,
    visible_byte_end,
  };

  switch (syntax) {
  case VIM_SYNTAX_RUBY:
    run_ruby_highlight(&writer_context, text, text_byte_length);
    break;

  case VIM_SYNTAX_PYTHON:
    editor_python_highlight_run(
      (const uint8_t *)text,
      text_byte_length,
      draw_highlight_segment,
      &writer_context
    );
    break;

  default:
    canvas->draw_row_text(
      canvas->context,
      column,
      text + visible_byte_begin,
      visible_byte_end - visible_byte_begin,
      area512_theme_text_color(),
      area512_theme_background_color(),
      0
    );
    break;
  }
}
