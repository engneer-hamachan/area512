#include "core/markdown/row_writer.h"
#include "core/text/utf8.h"

#include <string.h>

static const char MARKDOWN_SPACES[MARKDOWN_COLUMNS_MAX + 1] =
  "                                                "
  "                                                ";

void
init_markdown_row_writer(
  MarkdownRowWriter *writer,
  VimCanvas *canvas,
  int width,
  int rows_remaining,
  int highlight_code_enabled
) {

  memset(writer, 0, sizeof(*writer));

  writer->canvas = canvas;
  writer->width = width > MARKDOWN_COLUMNS_MAX ? MARKDOWN_COLUMNS_MAX : width;
  writer->rows_remaining = rows_remaining;
  writer->wrap_text = 1;
  writer->highlight_code_enabled = highlight_code_enabled;
}

void
style_markdown_row_writer(
  MarkdownRowWriter *writer,
  int left_column,
  int wrap_text,
  int preformatted_text,
  uint32_t background
) {

  writer->left_column = left_column;
  writer->wrap_text = wrap_text;
  writer->preformatted_text = preformatted_text;
  writer->background = background;
}

void
draw_markdown_text_span(
  MarkdownRowWriter *writer,
  int column,
  const char *text,
  int byte_length,
  uint32_t foreground,
  uint32_t background
) {

  writer->canvas->draw_row_text(
    writer->canvas->context,
    column,
    text,
    byte_length,
    foreground,
    background,
    0
  );
}

void
begin_markdown_output_row(MarkdownRowWriter *writer, int start_column) {
  writer->canvas->clear_row(writer->canvas->context);

  if (writer->background)
    draw_markdown_text_span(writer, 0, MARKDOWN_SPACES, writer->width, 0, writer->background);

  writer->column = start_column;
  writer->start_column = start_column;
  writer->row_open = 1;
}

void
end_markdown_output_row(MarkdownRowWriter *writer) {
  if (!writer->row_open)
    return;

  writer->canvas->push_row(writer->canvas->context, writer->screen_row);

  writer->screen_row++;
  writer->rows_remaining--;
  writer->row_open = 0;
}

static int
wrap_markdown_output_row(MarkdownRowWriter *writer) {
  end_markdown_output_row(writer);

  if (writer->rows_remaining <= 0)
    return 0;

  begin_markdown_output_row(writer, writer->left_column);

  return 1;
}

static void
clear_pending_markdown_word(MarkdownRowWriter *writer) {
  writer->word_byte_length = 0;
  writer->word_columns = 0;
}

static void
flush_pending_markdown_word(MarkdownRowWriter *writer) {
  if (writer->word_byte_length == 0)
    return;

  if (writer->column + writer->word_columns > writer->width) {
    if (!writer->wrap_text || writer->column <= writer->start_column) {
      clear_pending_markdown_word(writer);
      return;
    }

    if (!wrap_markdown_output_row(writer)) {
      clear_pending_markdown_word(writer);
      return;
    }
  }

  draw_markdown_text_span(
    writer,
    writer->column,
    writer->word,
    writer->word_byte_length,
    writer->word_foreground,
    writer->word_background
  );

  writer->column += writer->word_columns;

  clear_pending_markdown_word(writer);
}

void
write_markdown_preformatted_text(
  MarkdownRowWriter *writer,
  const char *text,
  int byte_length,
  uint32_t foreground
) {

  int byte_offset = 0;

  while (byte_offset < byte_length) {
    if (writer->rows_remaining <= 0)
      return;

    int available_columns = writer->width - writer->column;
    int chunk_start_byte_offset = byte_offset;
    int chunk_columns = 0;

    while (byte_offset < byte_length) {
      int character_byte_length =
        vim_utf8_byte_length((uint8_t)text[byte_offset]);

      if (character_byte_length > byte_length - byte_offset)
        character_byte_length = byte_length - byte_offset;

      int character_columns = vim_cell_width((uint8_t)text[byte_offset]);

      if (chunk_columns + character_columns > available_columns)
        break;

      chunk_columns += character_columns;
      byte_offset += character_byte_length;
    }

    if (byte_offset > chunk_start_byte_offset)
      draw_markdown_text_span(
        writer,
        writer->column,
        text + chunk_start_byte_offset,
        byte_offset - chunk_start_byte_offset,
        foreground,
        0
      );

    writer->column += chunk_columns;

    if (byte_offset >= byte_length)
      return;

    if (!writer->wrap_text || !wrap_markdown_output_row(writer))
      return;
  }
}

void
write_markdown_wrapped_span(
  MarkdownRowWriter *writer,
  const char *text,
  int byte_length,
  uint32_t foreground,
  uint32_t background
) {

  if (byte_length <= 0)
    return;

  if (writer->preformatted_text) {
    write_markdown_preformatted_text(writer, text, byte_length, foreground);
    return;
  }

  if (writer->word_byte_length > 0 &&
      (foreground != writer->word_foreground || background != writer->word_background))
    flush_pending_markdown_word(writer);

  writer->word_foreground = foreground;
  writer->word_background = background;

  int capacity = writer->width - writer->left_column;

  if (capacity < 1)
    capacity = 1;

  int byte_offset = 0;

  while (byte_offset < byte_length) {
    if (writer->rows_remaining <= 0)
      return;

    char byte = text[byte_offset];

    int character_byte_length = vim_utf8_byte_length((uint8_t)byte);

    if (character_byte_length > byte_length - byte_offset)
      character_byte_length = byte_length - byte_offset;

    int character_columns = vim_cell_width((uint8_t)byte);

    if (byte == ' ' || byte == '\t') {
      flush_pending_markdown_word(writer);

      if (writer->column > writer->start_column) {
        if (writer->column + 1 <= writer->width)
          writer->column++;
        else if (!writer->wrap_text || !wrap_markdown_output_row(writer))
          return;
      }

      byte_offset += character_byte_length;
      continue;
    }

    if (character_columns > 1 ||
        writer->word_columns + character_columns > capacity ||
        writer->word_byte_length + character_byte_length > MARKDOWN_WORD_MAX)
      flush_pending_markdown_word(writer);

    memcpy(writer->word + writer->word_byte_length, text + byte_offset, (size_t)character_byte_length);

    writer->word_byte_length += character_byte_length;
    writer->word_columns += character_columns;

    if (character_columns > 1)
      flush_pending_markdown_word(writer);

    byte_offset += character_byte_length;
  }

  flush_pending_markdown_word(writer);
}
