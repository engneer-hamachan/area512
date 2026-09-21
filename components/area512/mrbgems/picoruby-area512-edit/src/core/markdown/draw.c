#include "core/markdown/draw.h"
#include "core/markdown/parse.h"
#include "core/markdown/row_writer.h"
#include "core/syntax/picoruby/highlight.h"
#include "port/area512_editor_canvas.h"

#include <stdint.h>

static const char MARKDOWN_DASHES[MARKDOWN_COLUMNS_MAX + 1] =
  "------------------------------------------------"
  "------------------------------------------------";

typedef struct {
  MarkdownRowWriter *writer;
  uint32_t plain_foreground;
} MarkdownInlineContext;

static void
write_markdown_inline_span(
  void *writer_context,
  const char *text,
  int byte_length,
  MarkdownInlineStyle style
) {

  MarkdownInlineContext *context = (MarkdownInlineContext *)writer_context;

  uint32_t foreground = style == MARKDOWN_INLINE_PLAIN
                          ? context->plain_foreground
                          : area512_theme_emphasis_color();

  write_markdown_wrapped_span(
    context->writer,
    text,
    byte_length,
    foreground,
    area512_theme_background_color()
  );
}

static void
write_markdown_inline(
  MarkdownRowWriter *writer,
  const char *text,
  int byte_length,
  uint32_t plain_foreground
) {

  MarkdownInlineContext context = {writer, plain_foreground};

  scan_markdown_inline(text, byte_length, write_markdown_inline_span, &context);
}

static uint32_t
select_heading_foreground(int level) {
  if (level <= 2)
    return area512_theme_emphasis_color();

  return area512_theme_text_color();
}

static int
select_heading_font_size(int level) {
  if (level <= 1)
    return EDIT_HEADING1_FONT_SIZE;

  if (level == 2)
    return EDIT_HEADING2_FONT_SIZE;

  return EDIT_HEADING3_FONT_SIZE;
}

void
draw_markdown_blank(MarkdownRowWriter *writer) {
  style_markdown_row_writer(writer, 0, 1, 0, EDIT_BODY_FONT_SIZE, 0);

  begin_markdown_output_row(writer, 0);
  end_markdown_output_row(writer);
}

static void
draw_markdown_rule(MarkdownRowWriter *writer) {
  style_markdown_row_writer(writer, 0, 0, 1, EDIT_BODY_FONT_SIZE, 0);

  begin_markdown_output_row(writer, 0);
  draw_markdown_text_span(
    writer,
    0,
    MARKDOWN_DASHES,
    writer->width,
    area512_theme_border_color(),
    area512_theme_background_color()
  );
  end_markdown_output_row(writer);
}

static void
write_markdown_code_span(
  void *writer_context,
  const char *text,
  int byte_length,
  uint32_t color
) {

  write_markdown_preformatted_text(
    (MarkdownRowWriter *)writer_context,
    text,
    byte_length,
    color
  );
}

static void
highlight_markdown_code_row(
  MarkdownRowWriter *writer,
  const MarkdownBlock *block
) {
  editor_highlight_context_t context;

  editor_highlight_init(
    &context,
    (const uint8_t *)block->content,
    block->content_byte_length,
    write_markdown_code_span,
    writer
  );

  editor_highlight_run(&context);
}

static void
draw_markdown_code(
  MarkdownRowWriter *writer,
  const MarkdownBlock *block,
  MarkdownCodeLanguage language
) {

  style_markdown_row_writer(writer, 1, 1, 1, EDIT_BODY_FONT_SIZE, 0);

  begin_markdown_output_row(writer, 1);

  if (language == MARKDOWN_CODE_RUBY && writer->highlight_code_enabled)
    highlight_markdown_code_row(writer, block);
  else
    write_markdown_preformatted_text(
      writer,
      block->content,
      block->content_byte_length,
      area512_theme_emphasis_color()
    );

  end_markdown_output_row(writer);
}

static void
draw_markdown_heading(MarkdownRowWriter *writer, const MarkdownBlock *block) {
  uint32_t foreground = select_heading_foreground(block->heading_level);
  int font_size = select_heading_font_size(block->heading_level);

  style_markdown_row_writer(writer, 0, 1, 0, font_size, 0);

  begin_markdown_output_row(writer, 0);

  write_markdown_inline(
    writer,
    block->content,
    block->content_byte_length,
    foreground
  );

  end_markdown_output_row(writer);
}

// Cells are squeezed rather than aligned: 40 columns cannot hold a real grid.
static void
draw_markdown_table(MarkdownRowWriter *writer, const MarkdownBlock *block) {
  style_markdown_row_writer(writer, 0, 0, 0, EDIT_BODY_FONT_SIZE, 0);

  begin_markdown_output_row(writer, 0);

  const char *content = block->content;
  int byte_length = block->content_byte_length;
  int byte_offset = content[0] == '|' ? 1 : 0;
  int is_first_cell = 1;

  while (byte_offset < byte_length) {
    int cell_start_byte_offset = byte_offset;

    while (byte_offset < byte_length && content[byte_offset] != '|')
      byte_offset++;

    int cell_end_byte_offset = byte_offset;

    while (cell_start_byte_offset < cell_end_byte_offset &&
           (content[cell_start_byte_offset] == ' ' ||
            content[cell_start_byte_offset] == '\t'))
      cell_start_byte_offset++;

    while (cell_end_byte_offset > cell_start_byte_offset &&
           (content[cell_end_byte_offset - 1] == ' ' ||
            content[cell_end_byte_offset - 1] == '\t'))
      cell_end_byte_offset--;

    if (cell_end_byte_offset > cell_start_byte_offset) {
      if (!is_first_cell)
        write_markdown_wrapped_span(
          writer,
          " | ",
          3,
          area512_theme_border_color(),
          area512_theme_background_color()
        );

      write_markdown_inline(
        writer,
        content + cell_start_byte_offset,
        cell_end_byte_offset - cell_start_byte_offset,
        area512_theme_text_color()
      );

      is_first_cell = 0;
    }

    byte_offset++;
  }

  end_markdown_output_row(writer);
}

static void
draw_markdown_text(MarkdownRowWriter *writer, const MarkdownBlock *block) {
  uint32_t marker_foreground = block->kind == MARKDOWN_BLOCK_QUOTE
                                 ? area512_theme_text_color()
                                 : area512_theme_emphasis_color();

  style_markdown_row_writer(
    writer,
    block->content_column,
    1,
    0,
    EDIT_BODY_FONT_SIZE,
    0
  );

  begin_markdown_output_row(writer, block->marker_column);

  if (block->marker_byte_length > 0) {
    draw_markdown_text_span(
      writer,
      writer->column,
      block->marker,
      block->marker_byte_length,
      marker_foreground,
      area512_theme_background_color()
    );

    writer->column += block->marker_byte_length;
  }

  writer->start_column = writer->column;

  write_markdown_inline(
    writer,
    block->content,
    block->content_byte_length,
    area512_theme_text_color()
  );

  end_markdown_output_row(writer);
}

void
draw_markdown_block(
  MarkdownRowWriter *writer,
  const char *bytes,
  int byte_length,
  MarkdownCodeLanguage language
) {

  if (writer->rows_remaining <= 0)
    return;

  MarkdownBlock block;

  parse_markdown_block(
    bytes,
    byte_length,
    language != MARKDOWN_CODE_NONE,
    &block
  );

  switch (block.kind) {
  case MARKDOWN_BLOCK_CODE_FENCE:
    break;

  case MARKDOWN_BLOCK_BLANK:
    draw_markdown_blank(writer);
    break;

  case MARKDOWN_BLOCK_RULE:
  case MARKDOWN_BLOCK_TABLE_RULE:
    draw_markdown_rule(writer);
    break;

  case MARKDOWN_BLOCK_CODE:
    draw_markdown_code(writer, &block, language);
    break;

  case MARKDOWN_BLOCK_HEADING:
    draw_markdown_heading(writer, &block);
    break;

  case MARKDOWN_BLOCK_TABLE_ROW:
    draw_markdown_table(writer, &block);
    break;

  default:
    draw_markdown_text(writer, &block);
    break;
  }
}
