#ifndef EDIT_RENDER_SCREEN_H
#define EDIT_RENDER_SCREEN_H

#include <stdint.h>

#include "core/buffer/buffer.h"

#define VIM_GUTTER_WIDTH 4

typedef struct {
  void *context;
  void (*clear_row)(void *context);
  void (*draw_row_text)(
    void *context,
    int column,
    const char *text,
    int byte_length,
    uint32_t foreground,
    uint32_t background,
    int inverse
  );
  void (*push_row)(void *context, int row_index);
  void (*draw_cursor)(
    void *context,
    int column,
    int row_index,
    int visible
  );
} VimCanvas;

typedef void (*vim_highlight_function)(
  VimCanvas *canvas,
  int column,
  const char *segment,
  int segment_byte_length
);
typedef void (*vim_footer_function)(void *vim_context, VimCanvas *canvas);
typedef void (*vim_draw_cursor_function)(void *vim_context, VimCanvas *canvas);

typedef enum {
  VIM_REDRAW_NONE = 0,
  VIM_REDRAW_CURSOR,
  VIM_REDRAW_FOOTER,
  VIM_REDRAW_CURRENT_LINE,
  VIM_REDRAW_ALL
} VimRedraw;

typedef struct {
  int width, height;
  int footer_height;
  int content_margin_height;
  int visual_offset;
  int visual_cursor_column, visual_cursor_row;
  int syntax_highlight;

  VimBuffer buffer;
  VimRedraw redraw_mode;

  vim_highlight_function highlight;
  vim_footer_function footer;
  void *footer_context;
  vim_draw_cursor_function draw_cursor;
  void *draw_cursor_context;
} VimScreen;

void vim_screen_init(VimScreen *screen, int width, int height);
void vim_screen_free(VimScreen *screen);
void vim_screen_calculate_cursor(VimScreen *screen);
void vim_screen_refresh_if_needed(VimScreen *screen, VimCanvas *canvas);

#endif
