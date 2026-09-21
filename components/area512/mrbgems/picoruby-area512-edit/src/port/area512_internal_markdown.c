#if defined(AREA512_EXT_DISPLAY) && defined(PICORB_VM_MRUBYC)

#include "area512_markdown.h"
#include "core/markdown/draw.h"
#include "core/markdown/parse.h"
#include "core/markdown/row_writer.h"
#include "port/area512_editor_canvas.h"

#include <stdio.h>
#include <stdlib.h>

#define MARKDOWN_LINE_BYTE_SIZE 2048
#define MARKDOWN_MEASURE_ROW_COUNT 0x3FFFFFFF

typedef struct {
  MarkdownRowWriter writer;
  int page_row_count;
  int page_top_row;
  int page_remaining_row_count;
  int target_page_top_row;
  int previous_page_top_row;
} InternalMarkdownPagination;

static char internal_markdown_path_buffer[AREA512_PATH_MAX];
static int internal_markdown_page_top_row;
static int internal_markdown_next_page_top_row;
static int is_internal_markdown_shown;
static int is_internal_markdown_end_of_file_read;

static void
draw_internal_markdown_error(void) {
  const char text[] = "no documents";
  int byte_length = sizeof(text) - 1;
  int display_pixel_width = area512_internal_display_width();
  int display_pixel_height = area512_internal_display_height();
  uint32_t background = area512_theme_background_color();

  area512_internal_display_fill_rect(
    0,
    0,
    display_pixel_width,
    display_pixel_height,
    background
  );

  area512_internal_display_text(
    (display_pixel_width -
     byte_length * editor_canvas_font_width(EDIT_BODY_FONT_SIZE)) / 2,
    (display_pixel_height - EDIT_BODY_FONT_SIZE) / 2,
    text,
    byte_length,
    EDIT_BODY_FONT_SIZE,
    area512_theme_emphasis_color(),
    background
  );
}

static void
ignore_measured_row(void *context) {
  (void)context;
}

static void
start_next_internal_markdown_page(InternalMarkdownPagination *pagination) {
  pagination->page_top_row +=
    pagination->page_row_count - pagination->page_remaining_row_count;

  pagination->page_remaining_row_count = pagination->page_row_count;

  if (pagination->page_top_row < pagination->target_page_top_row)
    pagination->previous_page_top_row = pagination->page_top_row;
}

static void
assign_pushed_row_to_internal_markdown_page(void *context, int row_index) {
  (void)row_index;

  InternalMarkdownPagination *pagination =
    (InternalMarkdownPagination *)context;

  int row_span = pagination->writer.row_span;

  if (row_span > pagination->page_remaining_row_count)
    start_next_internal_markdown_page(pagination);

  pagination->page_remaining_row_count -= row_span;

  if (pagination->page_remaining_row_count == 0)
    start_next_internal_markdown_page(pagination);
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
draw_internal_markdown_row_text(
  void *context,
  int column,
  const char *text,
  int byte_length,
  uint32_t foreground,
  uint32_t background,
  int inverse
) {

  InternalMarkdownPagination *pagination =
    (InternalMarkdownPagination *)context;

  MarkdownRowWriter *writer = &pagination->writer;

  if (writer->rows_remaining < writer->row_span)
    return;

  if (writer->rows_to_skip >= writer->row_span)
    return;

  if (inverse) {
    uint32_t previous_foreground = foreground;
    foreground = background;
    background = previous_foreground;
  }

  area512_internal_display_text(
    column * editor_canvas_font_width(writer->font_size),
    writer->screen_row * EDIT_ROW_HEIGHT,
    text,
    byte_length,
    writer->font_size,
    foreground,
    background
  );
}

static int
read_internal_markdown_line(FILE *file, char *line) {
  int byte_length = 0;
  int character;

  while ((character = fgetc(file)) != EOF) {
    if (character == '\n')
      break;

    if (
      byte_length >= MARKDOWN_LINE_BYTE_SIZE - 4 &&
      (character & 0xC0) != 0x80
    ) {

      ungetc(character, file);
      break;
    }

    line[byte_length++] = (char)character;

    if (byte_length == MARKDOWN_LINE_BYTE_SIZE)
      break;
  }

  if (character == EOF && byte_length == 0)
    return -1;

  return byte_length;
}

static void
draw_markdown_line_and_update_code_language(
  MarkdownRowWriter *writer,
  const char *bytes,
  int byte_length,
  MarkdownCodeLanguage *language
) {

  draw_markdown_block(writer, bytes, byte_length, *language);

  if (is_markdown_code_fence(bytes, byte_length))
    *language = *language == MARKDOWN_CODE_NONE
                  ? read_markdown_code_fence_language(bytes, byte_length)
                  : MARKDOWN_CODE_NONE;
}

static void
draw_internal_markdown(
  int *is_shown,
  int *is_end_of_file_read,
  int *next_page_top_row
) {

  *is_shown = 0;
  *is_end_of_file_read = 0;
  *next_page_top_row = internal_markdown_page_top_row;

  FILE *file = fopen(internal_markdown_path_buffer, "rb");
  char *line = file ? (char *)malloc(MARKDOWN_LINE_BYTE_SIZE) : NULL;

  if (!line) {
    if (file)
      fclose(file);

    draw_internal_markdown_error();

    return;
  }

  int display_pixel_width = area512_internal_display_width();
  int display_pixel_height = area512_internal_display_height();

  area512_internal_display_fill_rect(
    0,
    0,
    display_pixel_width,
    display_pixel_height,
    area512_theme_background_color()
  );

  int columns = display_pixel_width / EDIT_CHAR_WIDTH;
  int rows = display_pixel_height / EDIT_ROW_HEIGHT;

  InternalMarkdownPagination pagination = {
    .page_row_count = rows,
    .page_top_row = internal_markdown_page_top_row,
    .page_remaining_row_count = rows,
    .target_page_top_row = internal_markdown_page_top_row,
    .previous_page_top_row = 0,
  };

  VimCanvas canvas = {
    .context = &pagination,
    .clear_row = ignore_measured_row,
    .draw_row_text = draw_internal_markdown_row_text,
    .push_row = assign_pushed_row_to_internal_markdown_page,
  };

  init_markdown_row_writer(
    &pagination.writer,
    &canvas,
    columns,
    rows,
    0
  );

  pagination.writer.rows_to_skip = internal_markdown_page_top_row;

  MarkdownCodeLanguage language = MARKDOWN_CODE_NONE;
  *is_shown = 1;

  while (pagination.writer.rows_remaining > 0) {
    int byte_length = read_internal_markdown_line(file, line);

    if (byte_length < 0) {
      *is_end_of_file_read = 1;
      break;
    }

    draw_markdown_line_and_update_code_language(
      &pagination.writer,
      line,
      byte_length,
      &language
    );
  }

  *next_page_top_row = pagination.page_top_row;

  if (ferror(file))
    *is_shown = 0;

  if (fclose(file) != 0)
    *is_shown = 0;

  free(line);

  if (!*is_shown)
    draw_internal_markdown_error();
}

static int
measure_previous_internal_markdown_page_top_row(void) {
  FILE *file = fopen(internal_markdown_path_buffer, "rb");
  char *line = file ? (char *)malloc(MARKDOWN_LINE_BYTE_SIZE) : NULL;

  if (!line) {
    if (file)
      fclose(file);

    return 0;
  }

  int display_pixel_width = area512_internal_display_width();
  int display_pixel_height = area512_internal_display_height();
  int columns = display_pixel_width / EDIT_CHAR_WIDTH;
  int rows = display_pixel_height / EDIT_ROW_HEIGHT;

  InternalMarkdownPagination pagination = {
    .page_row_count = rows,
    .page_top_row = 0,
    .page_remaining_row_count = rows,
    .target_page_top_row = internal_markdown_page_top_row,
    .previous_page_top_row = 0,
  };

  VimCanvas measurement_canvas = {
    .context = &pagination,
    .clear_row = ignore_measured_row,
    .draw_row_text = ignore_measured_text,
    .push_row = assign_pushed_row_to_internal_markdown_page,
  };

  init_markdown_row_writer(
    &pagination.writer,
    &measurement_canvas,
    columns,
    MARKDOWN_MEASURE_ROW_COUNT,
    0
  );

  MarkdownCodeLanguage language = MARKDOWN_CODE_NONE;

  while (pagination.page_top_row < pagination.target_page_top_row) {
    int byte_length = read_internal_markdown_line(file, line);

    if (byte_length < 0)
      break;

    draw_markdown_line_and_update_code_language(
      &pagination.writer,
      line,
      byte_length,
      &language
    );
  }

  fclose(file);
  free(line);

  return pagination.previous_page_top_row;
}

int
show_internal_markdown(const char *path) {
  if (!area512_internal_display_begin())
    return 0;

  if (
    area512_resolve_data_path(
      path,
      internal_markdown_path_buffer,
      sizeof(internal_markdown_path_buffer)
    ) != 0
  ) {

    is_internal_markdown_shown = 0;
    draw_internal_markdown_error();
    area512_internal_display_end();

    return 0;
  }

  internal_markdown_page_top_row = 0;

  draw_internal_markdown(
    &is_internal_markdown_shown,
    &is_internal_markdown_end_of_file_read,
    &internal_markdown_next_page_top_row
  );

  area512_internal_display_end();

  return is_internal_markdown_shown;
}

void
show_next_internal_markdown_page(void) {
  if (!is_internal_markdown_shown)
    return;

  if (!area512_internal_display_begin())
    return;

  if (is_internal_markdown_shown && !is_internal_markdown_end_of_file_read) {
    internal_markdown_page_top_row = internal_markdown_next_page_top_row;

    draw_internal_markdown(
      &is_internal_markdown_shown,
      &is_internal_markdown_end_of_file_read,
      &internal_markdown_next_page_top_row
    );
  }

  area512_internal_display_end();
}

void
show_previous_internal_markdown_page(void) {
  if (!is_internal_markdown_shown)
    return;

  if (!area512_internal_display_begin())
    return;

  if (is_internal_markdown_shown && internal_markdown_page_top_row > 0) {
    internal_markdown_page_top_row =
      measure_previous_internal_markdown_page_top_row();

    draw_internal_markdown(
      &is_internal_markdown_shown,
      &is_internal_markdown_end_of_file_read,
      &internal_markdown_next_page_top_row
    );
  }

  area512_internal_display_end();
}

void
hide_internal_markdown(void) {
  if (!area512_internal_display_begin())
    return;

  is_internal_markdown_shown = 0;

  int display_pixel_width = area512_internal_display_width();
  int display_pixel_height = area512_internal_display_height();

  area512_internal_display_fill_rect(
    0,
    0,
    display_pixel_width,
    display_pixel_height,
    area512_theme_background_color()
  );

  area512_internal_display_end();
}

#endif
