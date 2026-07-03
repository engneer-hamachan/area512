#include "core/render/screen.h"
#include "core/text/utf8.h"
#include <string.h>

void
vim_screen_init(VimScreen *screen, int width, int height) {
  memset(screen, 0, sizeof(*screen));

  screen->width = width;
  screen->height = height;
  screen->footer_height = 0;
  screen->content_margin_height = 5;
  screen->visual_offset = 0;
  screen->redraw_mode = VIM_REDRAW_NONE;

  vim_buffer_init(&screen->buffer);
}

void
vim_screen_free(VimScreen *screen) {
  vim_buffer_free(&screen->buffer);
}

static int
divide_rounding_up(int numerator, int denominator) {
  return (numerator + denominator - 1) / denominator;
}

static int
wrapped_row_count_for_line(
  const char *line,
  int byte_length,
  int content_width
) {

  if (content_width <= 0)
    return 1;

  int display_width = vim_display_width(line, byte_length);
  int rows = divide_rounding_up(display_width, content_width);

  if (rows < 1)
    return 1;

  return rows;
}

static int
wrapped_rows_before_line(VimScreen *screen, int line_index, int content_width) {
  int rows = 0;

  for (int i = 0; i < line_index && i < screen->buffer.line_count; i++)
    rows += wrapped_row_count_for_line(
      screen->buffer.lines[i].bytes,
      screen->buffer.lines[i].byte_length,
      content_width
    );

  return rows;
}

static int
buffer_total_wrapped_rows(VimScreen *screen, int content_width) {
  int rows = 0;

  for (int i = 0; i < screen->buffer.line_count; i++)
    rows += wrapped_row_count_for_line(
      screen->buffer.lines[i].bytes,
      screen->buffer.lines[i].byte_length,
      content_width
    );

  return rows;
}

void
vim_screen_calculate_cursor(VimScreen *screen) {
  int content_width = screen->width - VIM_GUTTER_WIDTH;

  if (content_width <= 0)
    content_width = 1;

  VimBuffer *buffer = &screen->buffer;

  int visual_row =
    wrapped_rows_before_line(screen, buffer->cursor_line_index, content_width);

  const char *current_line = buffer->lines[buffer->cursor_line_index].bytes;

  int current_line_byte_length =
    buffer->lines[buffer->cursor_line_index].byte_length;

  int cursor_display_column =
    vim_byte_to_column(
      current_line,
      current_line_byte_length,
      buffer->cursor_byte_offset
    );

  screen->visual_cursor_row =
    visual_row + cursor_display_column / content_width + screen->visual_offset;

  screen->visual_cursor_column = cursor_display_column % content_width;

  if (screen->visual_cursor_column == 0 && buffer->cursor_byte_offset > 0 &&
      current_line_byte_length == buffer->cursor_byte_offset) {

    screen->visual_cursor_column = content_width;
    screen->visual_cursor_row -= 1;
  }
}

static void
measure_column_span_byte_range(
  const char *line,
  int byte_length,
  int start_column,
  int max_width,
  int *byte_begin,
  int *byte_end
) {

  int byte_start = vim_column_to_byte(line, byte_length, start_column);
  int byte_offset = byte_start, width = 0;

  while (byte_offset < byte_length) {
    int cell_width = vim_cell_width((uint8_t)line[byte_offset]);

    if (width + cell_width > max_width)
      break;

    width += cell_width;
    byte_offset += vim_utf8_byte_length((uint8_t)line[byte_offset]);
  }

  *byte_begin = byte_start;
  *byte_end = byte_offset;
}

static void
draw_plain_row_text(
  VimPaint *paint,
  int column,
  const char *text,
  int byte_length,
  int inverse
) {
  paint->draw_row_text(
    paint->paint_context,
    column,
    text,
    byte_length,
    0,
    0,
    inverse
  );
}

static void
draw_display_slice(
  VimPaint *paint,
  int column,
  const char *segment,
  int segment_byte_length,
  int start_column,
  int width,
  int inverse
) {
  if (width <= 0)
    return;
  int byte_begin, byte_end;
  measure_column_span_byte_range(
    segment,
    segment_byte_length,
    start_column,
    width,
    &byte_begin,
    &byte_end
  );
  if (byte_end > byte_begin)
    draw_plain_row_text(
      paint,
      column,
      segment + byte_begin,
      byte_end - byte_begin,
      inverse
    );
}

static void
draw_segment(
  VimScreen *screen,
  VimPaint *paint,
  int draw_column,
  int line_index,
  const char *line,
  int byte_length,
  int start_column,
  int max_width
) {
  int byte_begin, byte_end;
  measure_column_span_byte_range(
    line,
    byte_length,
    start_column,
    max_width,
    &byte_begin,
    &byte_end
  );
  const char *segment = line + byte_begin;
  int segment_byte_length = byte_end - byte_begin;
  VimBuffer *buffer = &screen->buffer;
  int has_selection = vim_buffer_has_selection(buffer);

  if (screen->syntax_highlight && !has_selection) {
    if (screen->highlight)
      screen->highlight(paint, draw_column, segment, segment_byte_length);
    else
      draw_plain_row_text(paint, draw_column, segment, segment_byte_length, 0);
    return;
  }
  if (!has_selection) {
    draw_plain_row_text(paint, draw_column, segment, segment_byte_length, 0);
    return;
  }

  int start_line_index, start_byte_offset, end_line_index, end_byte_offset;
  if (!vim_buffer_selection_range(
        buffer,
        &start_line_index,
        &start_byte_offset,
        &end_line_index,
        &end_byte_offset
      )) {
    draw_plain_row_text(paint, draw_column, segment, segment_byte_length, 0);
    return;
  }
  if (buffer->selection_mode == VIM_SELECTION_LINE) {
    int inverse =
      (line_index >= start_line_index && line_index <= end_line_index);
    draw_plain_row_text(
      paint,
      draw_column,
      segment,
      segment_byte_length,
      inverse
    );
    return;
  }
  if (line_index < start_line_index || line_index > end_line_index) {
    draw_plain_row_text(paint, draw_column, segment, segment_byte_length, 0);
    return;
  }

  int selection_start_column = 0;
  if (line_index == start_line_index)
    selection_start_column =
      vim_byte_to_column(line, byte_length, start_byte_offset);
  int line_display_width = vim_display_width(line, byte_length);
  int selection_end_column;
  if (line_index == end_line_index) {
    int character_width = vim_cell_width((uint8_t)line[end_byte_offset]);
    selection_end_column =
      vim_byte_to_column(line, byte_length, end_byte_offset) + character_width -
      1;
  } else {
    selection_end_column = line_display_width - 1;
  }
  int segment_display_width = vim_display_width(segment, segment_byte_length);
  int segment_end_column = start_column + segment_display_width - 1;
  if (selection_start_column < start_column)
    selection_start_column = start_column;
  if (selection_end_column > segment_end_column)
    selection_end_column = segment_end_column;
  if (selection_start_column > segment_end_column ||
      selection_end_column < start_column) {
    draw_plain_row_text(paint, draw_column, segment, segment_byte_length, 0);
    return;
  }
  int before_width = selection_start_column - start_column;
  draw_display_slice(
    paint,
    draw_column,
    segment,
    segment_byte_length,
    0,
    before_width,
    0
  );
  int selection_width = selection_end_column - selection_start_column + 1;
  draw_display_slice(
    paint,
    draw_column + before_width,
    segment,
    segment_byte_length,
    before_width,
    selection_width,
    1
  );
  int after_column = selection_end_column - start_column + 1;
  draw_display_slice(
    paint,
    draw_column + after_column,
    segment,
    segment_byte_length,
    after_column,
    segment_display_width - after_column,
    0
  );
}

static void
draw_gutter(VimPaint *paint, int line_index, int wrap_index) {
  if (wrap_index > 0)
    return;
  char number[8];
  int number_byte_length = 0, line_number = line_index + 1;
  char reversed[8];
  int reversed_byte_length = 0;
  while (line_number > 0 && reversed_byte_length < 7) {
    reversed[reversed_byte_length++] = (char)('0' + line_number % 10);
    line_number /= 10;
  }
  for (int i = 0; i < reversed_byte_length; i++)
    number[number_byte_length++] = reversed[reversed_byte_length - 1 - i];
  number[number_byte_length++] = ' ';

  char padded[VIM_GUTTER_WIDTH];
  int padded_index = 0;
  for (int i = 0; i < VIM_GUTTER_WIDTH - number_byte_length; i++)
    padded[padded_index++] = ' ';
  for (int i = 0; i < number_byte_length && padded_index < VIM_GUTTER_WIDTH;
       i++)
    padded[padded_index++] = number[i];
  draw_plain_row_text(paint, 0, padded, VIM_GUTTER_WIDTH, 0);
}

static void
adjust_scroll(VimScreen *screen) {
  int content_height = screen->height - screen->footer_height;
  int content_width = screen->width - VIM_GUTTER_WIDTH;
  if (content_width <= 0)
    content_width = 1;
  int margin = screen->content_margin_height;
  if (margin > (content_height - 1) / 2)
    margin = (content_height - 1) / 2;
  if (margin < 0)
    margin = 0;
  vim_screen_calculate_cursor(screen);
  int offset;
  if ((offset = screen->visual_cursor_row - margin) < 0) {
    screen->visual_offset -= offset;
    if (screen->visual_offset > 0)
      screen->visual_offset = 0;
    vim_screen_calculate_cursor(screen);
  } else if ((offset =
                content_height - margin - screen->visual_cursor_row - 1) < 0) {
    screen->visual_offset += offset;
    vim_screen_calculate_cursor(screen);
  }
  int total_rows = buffer_total_wrapped_rows(screen, content_width);
  int max_scroll = total_rows - content_height;
  if (max_scroll < 0)
    max_scroll = 0;
  if (screen->visual_offset < -max_scroll) {
    screen->visual_offset = -max_scroll;
    vim_screen_calculate_cursor(screen);
  }
}

static void
vim_screen_refresh_full(VimScreen *screen, VimPaint *paint) {
  int content_height = screen->height - screen->footer_height;
  int content_width = screen->width - VIM_GUTTER_WIDTH;
  if (content_width <= 0)
    content_width = 1;

  adjust_scroll(screen);

  int visual_offset = screen->visual_offset;
  int screen_row = 0;

  int line_index = 0;
  while (line_index < screen->buffer.line_count) {
    const char *line = screen->buffer.lines[line_index].bytes;
    int byte_length = screen->buffer.lines[line_index].byte_length;
    int wrapped_row_count =
      wrapped_row_count_for_line(line, byte_length, content_width);
    int wrap_index = 0;
    while (wrap_index < wrapped_row_count) {
      if (visual_offset < 0) {
        visual_offset += 1;
      } else {
        paint->clear_row(paint->paint_context);
        draw_gutter(paint, line_index, wrap_index);
        draw_segment(
          screen,
          paint,
          VIM_GUTTER_WIDTH,
          line_index,
          line,
          byte_length,
          wrap_index * content_width,
          content_width
        );
        paint->push_row(paint->paint_context, screen_row);
        screen_row += 1;
        content_height -= 1;
        if (content_height == 0)
          break;
      }
      wrap_index += 1;
    }
    if (content_height == 0)
      break;
    line_index += 1;
  }
  while (content_height > 0) {
    paint->clear_row(paint->paint_context);
    paint->push_row(paint->paint_context, screen_row);
    screen_row += 1;
    content_height -= 1;
  }

  if (screen->footer)
    screen->footer(screen->footer_context, paint);
  if (screen->draw_cursor)
    screen->draw_cursor(screen->draw_cursor_context, paint);
}

static VimRedraw
convert_dirty_to_redraw_mode(VimDirty dirty) {
  switch (dirty) {
  case VIM_DIRTY_NONE:
    return VIM_REDRAW_NONE;
  case VIM_DIRTY_CURSOR:
    return VIM_REDRAW_CURSOR;
  case VIM_DIRTY_CONTENT:
    return VIM_REDRAW_CURRENT_LINE;
  default:
    return VIM_REDRAW_ALL;
  }
}

void
vim_screen_refresh_if_needed(VimScreen *screen, VimPaint *paint) {
  VimRedraw mode = convert_dirty_to_redraw_mode(screen->buffer.dirty);
  if (screen->redraw_mode != VIM_REDRAW_NONE)
    mode = screen->redraw_mode;
  screen->redraw_mode = VIM_REDRAW_NONE;
  vim_buffer_clear_dirty(&screen->buffer);
  if (mode == VIM_REDRAW_NONE)
    return;
  vim_screen_refresh_full(screen, paint);
}
