#ifndef EDIT_MARKDOWN_PARSE_H
#define EDIT_MARKDOWN_PARSE_H

#define MARKDOWN_MARKER_MAX 12

typedef enum {
  MARKDOWN_CODE_NONE = 0,
  MARKDOWN_CODE_PLAIN,
  MARKDOWN_CODE_RUBY
} MarkdownCodeLanguage;

typedef enum {
  MARKDOWN_BLOCK_BLANK = 0,
  MARKDOWN_BLOCK_PARAGRAPH,
  MARKDOWN_BLOCK_HEADING,
  MARKDOWN_BLOCK_LIST,
  MARKDOWN_BLOCK_QUOTE,
  MARKDOWN_BLOCK_RULE,
  MARKDOWN_BLOCK_CODE,
  MARKDOWN_BLOCK_CODE_FENCE,
  MARKDOWN_BLOCK_TABLE_ROW,
  MARKDOWN_BLOCK_TABLE_RULE
} MarkdownBlockKind;

typedef struct {
  MarkdownBlockKind kind;
  int heading_level;

  // Column the marker is drawn at, and the column wrapped rows continue from.
  int marker_column;
  int content_column;

  char marker[MARKDOWN_MARKER_MAX];
  int marker_byte_length;

  // Points into the parsed line; the viewer never takes ownership.
  const char *content;
  int content_byte_length;
} MarkdownBlock;

typedef enum {
  MARKDOWN_INLINE_PLAIN = 0,
  MARKDOWN_INLINE_STRONG,
  MARKDOWN_INLINE_EMPHASIS,
  MARKDOWN_INLINE_CODE,
  MARKDOWN_INLINE_LINK
} MarkdownInlineStyle;

typedef void (*markdown_span_writer)(
  void *writer_context,
  const char *text,
  int byte_length,
  MarkdownInlineStyle style
);

int is_markdown_code_fence(const char *line, int byte_length);

MarkdownCodeLanguage read_markdown_code_fence_language(
  const char *line,
  int byte_length
);

void parse_markdown_block(
  const char *line,
  int byte_length,
  int inside_code_fence,
  MarkdownBlock *block
);

void scan_markdown_inline(
  const char *text,
  int byte_length,
  markdown_span_writer write_span,
  void *writer_context
);

#endif
