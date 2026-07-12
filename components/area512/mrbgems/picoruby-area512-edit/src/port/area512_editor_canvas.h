#ifndef AREA512_EDITOR_CANVAS_H
#define AREA512_EDITOR_CANVAS_H

#include <stdint.h>

#include "area512_hal.h"
#include "core/render/screen.h"

#define EDIT_FONT_SIZE 12
#define EDIT_BODY_FONT_SIZE 12
#define EDIT_HEADING1_FONT_SIZE 24
#define EDIT_HEADING2_FONT_SIZE 16
#define EDIT_HEADING3_FONT_SIZE 14
#define EDIT_CHAR_WIDTH 6
#define EDIT_ROW_HEIGHT 13
#define EDIT_FOREGROUND 0xF5972D
#define EDIT_BACKGROUND 0x000000
#define EDIT_CURSOR_KEY 0x000000

typedef struct {
  void *row_sprite;
  void *cursor_sprite;
  int char_width;
  int row_height;
  int font_size;
} Area512EditorCanvas;

void clear_editor_canvas_row(void *context);

void draw_editor_canvas_row_text(
  void *context,
  int column,
  const char *text,
  int byte_length,
  uint32_t foreground,
  uint32_t background,
  int inverse
);

void push_editor_canvas_row(void *context, int row_index);

int editor_canvas_font_width(int font_size);
int editor_canvas_font_row_span(int font_size);
int editor_canvas_font_row_height(int font_size);

void set_editor_canvas_font_size(void *context, int font_size);

void draw_editor_canvas_cursor(
  void *context,
  int column,
  int row_index,
  int visible
);

void highlight_edit_segment(
  VimCanvas *canvas,
  int column,
  const char *segment,
  int segment_byte_length
);

#endif
