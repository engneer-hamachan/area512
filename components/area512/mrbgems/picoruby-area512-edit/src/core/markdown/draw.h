#ifndef EDIT_MARKDOWN_DRAW_H
#define EDIT_MARKDOWN_DRAW_H

#include "core/markdown/parse.h"
#include "core/markdown/row_writer.h"

void draw_markdown_blank(MarkdownRowWriter *writer);

void draw_markdown_block(
  MarkdownRowWriter *writer,
  const char *bytes,
  int byte_length,
  MarkdownCodeLanguage language
);

#endif
