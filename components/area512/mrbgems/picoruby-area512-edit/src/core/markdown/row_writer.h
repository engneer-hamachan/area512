#ifndef EDIT_MARKDOWN_ROW_WRITER_H
#define EDIT_MARKDOWN_ROW_WRITER_H

#include <stdint.h>

#include "core/render/screen.h"

#define MARKDOWN_COLUMNS_MAX 96
#define MARKDOWN_WORD_MAX 48

typedef struct {
  VimCanvas *canvas;

  int width;
  int left_column;
  int start_column;
  int column;
  int screen_row;
  int rows_remaining;
  int row_open;

  int wrap_text;
  int preformatted_text;

  // Off during measurement: token colors never change how many rows a line
  // occupies, and the Ruby lexer is far too slow to run per scroll step.
  int highlight_code_enabled;

  uint32_t background;

  char word[MARKDOWN_WORD_MAX];
  int word_byte_length;
  int word_columns;
  uint32_t word_foreground;
  uint32_t word_background;
} MarkdownRowWriter;

void init_markdown_row_writer(
  MarkdownRowWriter *writer,
  VimCanvas *canvas,
  int width,
  int rows_remaining,
  int highlight_code_enabled
);

void style_markdown_row_writer(
  MarkdownRowWriter *writer,
  int left_column,
  int wrap_text,
  int preformatted_text,
  uint32_t background
);

void draw_markdown_text_span(
  MarkdownRowWriter *writer,
  int column,
  const char *text,
  int byte_length,
  uint32_t foreground,
  uint32_t background
);

void begin_markdown_output_row(MarkdownRowWriter *writer, int start_column);
void end_markdown_output_row(MarkdownRowWriter *writer);

void write_markdown_preformatted_text(
  MarkdownRowWriter *writer,
  const char *text,
  int byte_length,
  uint32_t foreground
);

void write_markdown_wrapped_span(
  MarkdownRowWriter *writer,
  const char *text,
  int byte_length,
  uint32_t foreground,
  uint32_t background
);

#endif
