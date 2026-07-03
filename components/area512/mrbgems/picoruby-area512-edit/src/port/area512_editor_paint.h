#ifndef AREA512_EDITOR_PAINT_H
#define AREA512_EDITOR_PAINT_H

#include <stdint.h>

#include "area512_hal.h"
#include "core/render/screen.h"

#define EDIT_FONT_SIZE 12
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
} editor_paint;

void clear_editor_paint_row(void *paint_context);

void draw_editor_paint_row_text(
  void *paint_context,
  int column,
  const char *text,
  int byte_length,
  uint32_t foreground,
  uint32_t background,
  int inverse
);

void push_editor_paint_row(void *paint_context, int row_index);

void draw_editor_paint_cursor(
  void *paint_context,
  int column,
  int row_index,
  int visible
);

void highlight_edit_segment(
  VimPaint *paint,
  int column,
  const char *segment,
  int segment_byte_length
);

#endif
