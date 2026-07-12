#include "core/markdown/viewer.h"
#include "core/markdown/parse.h"
#include "core/markdown/row_writer.h"
#include "core/markdown/draw.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MARKDOWN_FOOTER_TEXT 0xFFFFFF
#define MARKDOWN_FOOTER_BACKGROUND 0x4E4E4E

#define MARKDOWN_MEASURE_ROWS 0x3FFFFFFF

// -----------------------------------------------------------------------------
// Scrolling
// -----------------------------------------------------------------------------

static void
ignore_measured_row(void *context) {
  (void)context;
}

static void
ignore_measured_text(
  void *context,
  int column,
  const char *text,
  int byte_length,
  uint32_t foreground,
  uint32_t background,
  int inverse
) {

  (void)context;
  (void)column;
  (void)text;
  (void)byte_length;
  (void)foreground;
  (void)background;
  (void)inverse;
}

static void
count_measured_push(void *context, int row_index) {
  (void)row_index;

  if (context)
    (*(int *)context)++;
}

static int
measure_visible_content_rows(MarkdownViewer *viewer) {
  int visible_rows = viewer->height - viewer->footer_height;

  return visible_rows < 1 ? 1 : visible_rows;
}

static int
measure_markdown_line(MarkdownViewer *viewer, int line_index, int *output_rows) {
  int pushed_rows = 0;
  VimCanvas measurement_canvas = {
    .context = &pushed_rows,
    .clear_row = ignore_measured_row,
    .draw_row_text = ignore_measured_text,
    .push_row = count_measured_push,
    .draw_cursor = NULL,
  };

  MarkdownRowWriter writer;
  VimString *line = &viewer->buffer.lines[line_index];

  init_markdown_row_writer(
    &writer,
    &measurement_canvas,
    viewer->width,
    MARKDOWN_MEASURE_ROWS,
    0
  );

  draw_markdown_block(
    &writer,
    line->bytes,
    line->byte_length,
    (MarkdownCodeLanguage)viewer->code_language[line_index]
  );

  if (output_rows)
    *output_rows = pushed_rows;

  return writer.screen_row;
}

static int
measure_markdown_line_rows(MarkdownViewer *viewer, int line_index) {
  return measure_markdown_line(viewer, line_index, NULL);
}

static int
measure_markdown_line_row_span(MarkdownViewer *viewer, int line_index) {
  int output_rows = 0;
  int screen_rows = measure_markdown_line(viewer, line_index, &output_rows);

  return output_rows > 0 ? screen_rows / output_rows : 1;
}

static int
measure_markdown_total_rows(MarkdownViewer *viewer) {
  int rows = 0;

  for (int line_index = 0; line_index < viewer->buffer.line_count; line_index++)
    rows += measure_markdown_line_rows(viewer, line_index);

  return rows;
}

static void
set_markdown_top_row(MarkdownViewer *viewer, int top_row) {
  int visible_rows = measure_visible_content_rows(viewer);
  int max_top_row = measure_markdown_total_rows(viewer) - visible_rows;

  if (max_top_row < 0)
    max_top_row = 0;
  if (top_row < 0)
    top_row = 0;
  if (top_row > max_top_row)
    top_row = max_top_row;

  viewer->top_line = 0;
  viewer->top_line_row = top_row;

  while (viewer->top_line < viewer->buffer.line_count) {
    int line_rows = measure_markdown_line_rows(viewer, viewer->top_line);

    if (viewer->top_line_row < line_rows)
      break;

    viewer->top_line_row -= line_rows;
    viewer->top_line++;
  }

  if (viewer->top_line < viewer->buffer.line_count) {
    int row_span = measure_markdown_line_row_span(viewer, viewer->top_line);
    viewer->top_line_row -= viewer->top_line_row % row_span;
  }
}

static int
get_markdown_top_row(MarkdownViewer *viewer) {
  int top_row = viewer->top_line_row;

  for (int line_index = 0; line_index < viewer->top_line; line_index++)
    top_row += measure_markdown_line_rows(viewer, line_index);

  return top_row;
}

static void
scroll_markdown_lines(MarkdownViewer *viewer, int direction) {
  int row_span = 1;

  if (viewer->top_line < viewer->buffer.line_count)
    row_span = measure_markdown_line_row_span(viewer, viewer->top_line);

  set_markdown_top_row(
    viewer,
    get_markdown_top_row(viewer) + direction * row_span
  );
}

static void
scroll_markdown_page(MarkdownViewer *viewer, int direction) {
  int visible_rows = measure_visible_content_rows(viewer);
  int page_rows = visible_rows > 1 ? visible_rows - 1 : 1;
  set_markdown_top_row(
    viewer,
    get_markdown_top_row(viewer) + direction * page_rows
  );
}

// -----------------------------------------------------------------------------
// Footer
// -----------------------------------------------------------------------------

// VimString is not NUL-terminated, so the name is copied out before printing.
static void
copy_markdown_base_name(
  const VimString *filepath,
  char *destination,
  int destination_size
) {

  int base_name_byte_offset = 0;

  for (int i = 0; i < filepath->byte_length; i++)
    if (filepath->bytes[i] == '/')
      base_name_byte_offset = i + 1;

  int byte_length = filepath->byte_length - base_name_byte_offset;

  if (byte_length < 0)
    byte_length = 0;

  if (byte_length > destination_size - 1)
    byte_length = destination_size - 1;

  if (byte_length > 0)
    memcpy(
      destination,
      filepath->bytes + base_name_byte_offset,
      (size_t)byte_length
    );

  destination[byte_length] = 0;
}

static int
measure_markdown_scroll_percent(MarkdownViewer *viewer) {
  int max_top_row =
    measure_markdown_total_rows(viewer) - measure_visible_content_rows(viewer);

  if (max_top_row <= 0)
    return 100;

  return get_markdown_top_row(viewer) * 100 / max_top_row;
}

static void
draw_markdown_footer(MarkdownViewer *viewer, VimCanvas *canvas) {
  char text[MARKDOWN_COLUMNS_MAX + 1];
  char name[MARKDOWN_COLUMNS_MAX + 1];

  int width = viewer->width;

  if (width > MARKDOWN_COLUMNS_MAX)
    width = MARKDOWN_COLUMNS_MAX;

  copy_markdown_base_name(&viewer->filepath, name, sizeof(name));

  snprintf(
    text,
    sizeof(text),
    " %s  q:back  %d%%",
    name,
    measure_markdown_scroll_percent(viewer)
  );

  int byte_length = (int)strlen(text);

  while (byte_length < width && byte_length < MARKDOWN_COLUMNS_MAX)
    text[byte_length++] = ' ';

  text[byte_length] = 0;

  canvas->clear_row(canvas->context);

  canvas->draw_row_text(
    canvas->context,
    0,
    text,
    byte_length,
    MARKDOWN_FOOTER_TEXT,
    MARKDOWN_FOOTER_BACKGROUND,
    0
  );

  canvas->push_row(canvas->context, viewer->height - viewer->footer_height);
}

// -----------------------------------------------------------------------------
// Lifecycle, draw, input
// -----------------------------------------------------------------------------

void
init_markdown_viewer(MarkdownViewer *viewer, int width, int height) {
  memset(viewer, 0, sizeof(*viewer));

  viewer->width = width;
  viewer->height = height;
  viewer->footer_height = 1;

  vim_buffer_init(&viewer->buffer);
  vim_string_init(&viewer->filepath);
}

void
free_markdown_viewer(MarkdownViewer *viewer) {
  vim_buffer_free(&viewer->buffer);
  vim_string_free(&viewer->filepath);

  if (viewer->code_language)
    free(viewer->code_language);

  viewer->code_language = NULL;
}

void
set_markdown_viewer_filepath(
  MarkdownViewer *viewer,
  const char *text,
  int byte_length
) {

  vim_string_set(&viewer->filepath, text, byte_length);
}

void
load_markdown_viewer_text(
  MarkdownViewer *viewer,
  const char *text,
  int byte_length
) {

  vim_buffer_load_text(&viewer->buffer, text, byte_length);

  if (viewer->code_language)
    free(viewer->code_language);

  viewer->top_line = 0;
  viewer->code_language =
    (uint8_t *)calloc((size_t)viewer->buffer.line_count, sizeof(uint8_t));

  if (!viewer->code_language)
    return;

  MarkdownCodeLanguage language = MARKDOWN_CODE_NONE;

  for (int line_index = 0; line_index < viewer->buffer.line_count; line_index++) {
    VimString *line = &viewer->buffer.lines[line_index];

    viewer->code_language[line_index] = (uint8_t)language;

    if (is_markdown_code_fence(line->bytes, line->byte_length))
      language = language == MARKDOWN_CODE_NONE
                   ? read_markdown_code_fence_language(line->bytes, line->byte_length)
                   : MARKDOWN_CODE_NONE;
  }
}

void
draw_markdown_viewer(MarkdownViewer *viewer, VimCanvas *canvas) {
  if (!viewer->code_language)
    return;

  MarkdownRowWriter writer;

  init_markdown_row_writer(
    &writer,
    canvas,
    viewer->width,
    measure_visible_content_rows(viewer),
    1
  );

  writer.rows_to_skip = viewer->top_line_row;

  int line_index = viewer->top_line;

  while (writer.rows_remaining > 0 && line_index < viewer->buffer.line_count) {
    VimString *line = &viewer->buffer.lines[line_index];

    draw_markdown_block(
      &writer,
      line->bytes,
      line->byte_length,
      (MarkdownCodeLanguage)viewer->code_language[line_index]
    );

    line_index++;
  }

  while (writer.rows_remaining > 0)
    draw_markdown_blank(&writer);

  draw_markdown_footer(viewer, canvas);
}

MarkdownViewerStatus
handle_markdown_viewer_key(MarkdownViewer *viewer, int key_byte) {
  switch (key_byte) {
  case 'q':
  case ',':
  case 'u':
  case '\b':
  case 127:
  case 3:
    return MARKDOWN_VIEWER_QUIT;

  case 'j':
  case '.':
    scroll_markdown_lines(viewer, 1);
    break;

  case 'k':
  case ';':
    scroll_markdown_lines(viewer, -1);
    break;

  case ' ':
  case 'f':
  case '/':
    scroll_markdown_page(viewer, 1);
    break;

  case 'b':
    scroll_markdown_page(viewer, -1);
    break;

  case 'g':
    viewer->top_line = 0;
    viewer->top_line_row = 0;
    break;

  case 'G':
    set_markdown_top_row(viewer, measure_markdown_total_rows(viewer));
    break;
  }

  return MARKDOWN_VIEWER_CONTINUE;
}

MarkdownViewerStatus
handle_markdown_viewer_escape(
  MarkdownViewer *viewer,
  const char *sequence,
  int byte_length
) {

  if (byte_length < 2 || sequence[0] != '[')
    return MARKDOWN_VIEWER_QUIT;

  switch (sequence[1]) {
  case 'A':
    scroll_markdown_lines(viewer, -1);
    break;

  case 'B':
    scroll_markdown_lines(viewer, 1);
    break;

  case 'C':
    scroll_markdown_page(viewer, 1);
    break;

  case 'D':
    return MARKDOWN_VIEWER_QUIT;
  }

  return MARKDOWN_VIEWER_CONTINUE;
}
