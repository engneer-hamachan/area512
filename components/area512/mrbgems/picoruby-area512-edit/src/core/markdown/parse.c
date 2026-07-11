#include "core/markdown/parse.h"
#include "core/text/utf8.h"

#include <stdint.h>
#include <string.h>

#define TAB_COLUMNS 2
#define MARKDOWN_HEADING_LEVEL_MAX 6

static int
is_space(char byte) {
  return byte == ' ' || byte == '\t';
}

static int
is_digit(char byte) {
  return byte >= '0' && byte <= '9';
}

static int
is_markdown_escapable_ascii(char byte) {
  return (byte >= '!' && byte <= '/') || (byte >= ':' && byte <= '@') ||
         (byte >= '[' && byte <= '`') || (byte >= '{' && byte <= '~');
}

static int
find_indented_text_byte_offset(
  const char *line,
  int byte_length,
  int *indent_columns
) {
  int byte_offset = 0, column = 0;

  while (byte_offset < byte_length && is_space(line[byte_offset])) {
    column += line[byte_offset] == '\t' ? TAB_COLUMNS : 1;
    byte_offset++;
  }

  *indent_columns = column;

  return byte_offset;
}

static int
trim_trailing_space_byte_length(const char *line, int byte_length) {
  while (byte_length > 0 &&
         (is_space(line[byte_length - 1]) || line[byte_length - 1] == '\r'))
    byte_length--;

  return byte_length;
}

int
is_markdown_code_fence(const char *line, int byte_length) {
  int indent_columns;
  int byte_offset =
    find_indented_text_byte_offset(line, byte_length, &indent_columns);

  if (byte_length - byte_offset < 3)
    return 0;

  char marker = line[byte_offset];

  if (marker != '`' && marker != '~')
    return 0;

  return line[byte_offset + 1] == marker && line[byte_offset + 2] == marker;
}

// An unlabelled or unrecognised fence is MARKDOWN_CODE_PLAIN, never NONE: the
// caller uses NONE to mean "outside a fence".
MarkdownCodeLanguage
read_markdown_code_fence_language(const char *line, int byte_length) {
  if (!is_markdown_code_fence(line, byte_length))
    return MARKDOWN_CODE_NONE;

  int indent_columns;
  int byte_offset =
    find_indented_text_byte_offset(line, byte_length, &indent_columns);
  char marker = line[byte_offset];

  while (byte_offset < byte_length && line[byte_offset] == marker)
    byte_offset++;

  while (byte_offset < byte_length && is_space(line[byte_offset]))
    byte_offset++;

  int language_byte_offset = byte_offset;

  while (byte_offset < byte_length && !is_space(line[byte_offset]) &&
         line[byte_offset] != '\r')
    byte_offset++;

  int language_byte_length = byte_offset - language_byte_offset;

  if ((language_byte_length == 4 &&
       memcmp(line + language_byte_offset, "ruby", 4) == 0) ||
      (language_byte_length == 2 &&
       memcmp(line + language_byte_offset, "rb", 2) == 0))
    return MARKDOWN_CODE_RUBY;

  return MARKDOWN_CODE_PLAIN;
}

static int
is_thematic_break(const char *line, int byte_length) {
  int indent_columns;
  int byte_offset =
    find_indented_text_byte_offset(line, byte_length, &indent_columns);
  char marker = 0;
  int count = 0;

  for (int i = byte_offset; i < byte_length; i++) {
    char byte = line[i];

    if (is_space(byte))
      continue;

    if (byte != '-' && byte != '*' && byte != '_')
      return 0;

    if (marker && byte != marker)
      return 0;

    marker = byte;
    count++;
  }

  return count >= 3;
}

// A table rule is the |---|:--:| row directly under a table header.
static int
is_table_rule(const char *line, int byte_length, int byte_offset) {
  int dashes = 0;

  for (int i = byte_offset; i < byte_length; i++) {
    char byte = line[i];

    if (byte == '-')
      dashes++;
    else if (byte != '|' && byte != ':' && !is_space(byte))
      return 0;
  }

  return dashes > 0;
}

static int
is_bullet_list(const char *line, int byte_length, int byte_offset) {
  char byte = line[byte_offset];

  if (byte != '-' && byte != '*' && byte != '+')
    return 0;

  return byte_offset + 1 < byte_length && is_space(line[byte_offset + 1]);
}

// Byte length of "12." / "3)" including the delimiter, or 0 when not ordered.
static int
measure_ordered_list_marker_byte_length(
  const char *line,
  int byte_length,
  int byte_offset
) {
  int i = byte_offset;

  while (i < byte_length && is_digit(line[i]))
    i++;

  if (i == byte_offset || i >= byte_length)
    return 0;

  if (line[i] != '.' && line[i] != ')')
    return 0;

  if (i + 1 >= byte_length || !is_space(line[i + 1]))
    return 0;

  return i + 1 - byte_offset;
}

static void
set_marker(MarkdownBlock *block, const char *text, int byte_length) {
  if (byte_length > MARKDOWN_MARKER_MAX - 1)
    byte_length = MARKDOWN_MARKER_MAX - 1;

  memcpy(block->marker, text, (size_t)byte_length);

  block->marker[byte_length] = 0;
  block->marker_byte_length = byte_length;
}

static void
set_content(MarkdownBlock *block, const char *text, int byte_length) {
  block->content = text;
  block->content_byte_length = byte_length;
}

static void
set_bullet_marker(MarkdownBlock *block) {
  set_marker(block, "- ", 2);
}

void
parse_markdown_block(
  const char *line,
  int byte_length,
  int inside_code_fence,
  MarkdownBlock *block
) {

  memset(block, 0, sizeof(*block));

  set_content(block, line, byte_length);

  int code_fence = is_markdown_code_fence(line, byte_length);

  if (inside_code_fence) {
    block->kind = code_fence ? MARKDOWN_BLOCK_CODE_FENCE : MARKDOWN_BLOCK_CODE;

    set_content(block, line, trim_trailing_space_byte_length(line, byte_length));

    return;
  } else if (code_fence) {
    block->kind = MARKDOWN_BLOCK_CODE_FENCE;

    return;
  }

  byte_length = trim_trailing_space_byte_length(line, byte_length);

  int indent_columns;
  int byte_offset =
    find_indented_text_byte_offset(line, byte_length, &indent_columns);

  if (byte_offset == byte_length) {
    block->kind = MARKDOWN_BLOCK_BLANK;

    set_content(block, line, 0);

    return;
  }

  block->marker_column = indent_columns;
  block->content_column = indent_columns;

  set_content(block, line + byte_offset, byte_length - byte_offset);

  if (is_thematic_break(line, byte_length)) {
    block->kind = MARKDOWN_BLOCK_RULE;

    return;
  }

  if (line[byte_offset] == '#') {
    int level = 0;

    while (byte_offset + level < byte_length && line[byte_offset + level] == '#')
      level++;

    int heading_text_byte_offset = byte_offset + level;

    if (level <= MARKDOWN_HEADING_LEVEL_MAX &&
        (heading_text_byte_offset == byte_length ||
         is_space(line[heading_text_byte_offset]))) {

      while (heading_text_byte_offset < byte_length &&
             is_space(line[heading_text_byte_offset]))
        heading_text_byte_offset++;

      block->kind = MARKDOWN_BLOCK_HEADING;
      block->heading_level = level;
      block->marker_column = 0;
      block->content_column = 0;

      set_content(
        block,
        line + heading_text_byte_offset,
        byte_length - heading_text_byte_offset
      );

      return;
    }
  }

  if (line[byte_offset] == '|') {
    block->kind = MARKDOWN_BLOCK_TABLE_ROW;

    if (is_table_rule(line, byte_length, byte_offset))
      block->kind = MARKDOWN_BLOCK_TABLE_RULE;

    block->marker_column = 0;
    block->content_column = 0;

    return;
  }

  if (line[byte_offset] == '>') {
    int quote_content_byte_offset = byte_offset + 1;

    if (quote_content_byte_offset < byte_length &&
        line[quote_content_byte_offset] == ' ')
      quote_content_byte_offset++;

    block->kind = MARKDOWN_BLOCK_QUOTE;
    block->content_column = indent_columns + 2;

    set_marker(block, "> ", 2);
    set_content(
      block,
      line + quote_content_byte_offset,
      byte_length - quote_content_byte_offset
    );

    return;
  }

  if (is_bullet_list(line, byte_length, byte_offset)) {
    block->kind = MARKDOWN_BLOCK_LIST;
    block->content_column = indent_columns + 2;

    set_bullet_marker(block);
    set_content(block, line + byte_offset + 2, byte_length - byte_offset - 2);

    return;
  }

  int ordered_byte_length =
    measure_ordered_list_marker_byte_length(line, byte_length, byte_offset);

  if (ordered_byte_length > 0) {
    char marker[MARKDOWN_MARKER_MAX];

    int marker_byte_length = ordered_byte_length;

    if (marker_byte_length > MARKDOWN_MARKER_MAX - 2)
      marker_byte_length = MARKDOWN_MARKER_MAX - 2;

    memcpy(marker, line + byte_offset, (size_t)marker_byte_length);

    marker[marker_byte_length] = ' ';

    block->kind = MARKDOWN_BLOCK_LIST;
    block->content_column = indent_columns + marker_byte_length + 1;

    set_marker(block, marker, marker_byte_length + 1);

    set_content(
      block,
      line + byte_offset + ordered_byte_length + 1,
      byte_length - byte_offset - ordered_byte_length - 1
    );

    return;
  }

  block->kind = MARKDOWN_BLOCK_PARAGRAPH;
}

// -----------------------------------------------------------------------------
// Inline spans
// -----------------------------------------------------------------------------

static int
find_unescaped_byte_offset(
  const char *text,
  int byte_length,
  int start_byte_offset,
  char wanted_byte
) {
  for (int i = start_byte_offset; i < byte_length; i++) {
    if (text[i] == '\\') {
      i++;
      continue;
    }

    if (text[i] == wanted_byte)
      return i;
  }

  return -1;
}

static int
find_markdown_delimiter_byte_offset(
  const char *text,
  int byte_length,
  int start_byte_offset,
  char delimiter,
  int delimiter_byte_length
) {

  for (int i = start_byte_offset; i + delimiter_byte_length <= byte_length; i++) {
    if (text[i] == '\\') {
      i++;
      continue;
    }

    if (text[i] != delimiter)
      continue;

    if (delimiter_byte_length == 1) {
      if (i + 1 < byte_length && text[i + 1] == delimiter)
        continue;

      return i;
    }

    if (text[i + 1] == delimiter)
      return i;
  }

  return -1;
}

// [label](target) or ![label](target) starting at the '['; on success *label_end
// is the ']' offset and *link_end the offset just past the ')'.
static int
scan_markdown_link(
  const char *text,
  int byte_length,
  int opening_bracket_byte_offset,
  int *label_end_byte_offset,
  int *link_end_byte_offset
) {

  int closing_bracket_byte_offset = find_unescaped_byte_offset(
    text,
    byte_length,
    opening_bracket_byte_offset + 1,
    ']'
  );

  if (closing_bracket_byte_offset < 0)
    return 0;

  if (closing_bracket_byte_offset + 1 >= byte_length ||
      text[closing_bracket_byte_offset + 1] != '(')
    return 0;

  int closing_parenthesis_byte_offset = find_unescaped_byte_offset(
    text,
    byte_length,
    closing_bracket_byte_offset + 2,
    ')'
  );

  if (closing_parenthesis_byte_offset < 0)
    return 0;

  *label_end_byte_offset = closing_bracket_byte_offset;
  *link_end_byte_offset = closing_parenthesis_byte_offset + 1;

  return 1;
}

void
scan_markdown_inline(
  const char *text,
  int byte_length,
  markdown_span_writer write_span,
  void *writer_context
) {

  int plain_start_byte_offset = 0;
  int scan_byte_offset = 0;

  while (scan_byte_offset < byte_length) {
    char byte = text[scan_byte_offset];
    int span_start_byte_offset = -1;
    int span_byte_length = 0;
    int next_byte_offset = 0;
    MarkdownInlineStyle style = MARKDOWN_INLINE_PLAIN;

    if (byte == '\\' && scan_byte_offset + 1 < byte_length &&
        is_markdown_escapable_ascii(text[scan_byte_offset + 1])) {
      span_start_byte_offset = scan_byte_offset + 1;
      span_byte_length = 1;
      next_byte_offset = scan_byte_offset + 2;

    } else if (byte == '`') {
      int closing_byte_offset = find_unescaped_byte_offset(
        text,
        byte_length,
        scan_byte_offset + 1,
        '`'
      );

      if (closing_byte_offset > scan_byte_offset + 1) {
        span_start_byte_offset = scan_byte_offset + 1;
        span_byte_length = closing_byte_offset - scan_byte_offset - 1;
        style = MARKDOWN_INLINE_CODE;
        next_byte_offset = closing_byte_offset + 1;
      }

    } else if (byte == '*') {
      int is_strong_delimiter =
        scan_byte_offset + 1 < byte_length && text[scan_byte_offset + 1] == '*';
      int delimiter_byte_length = is_strong_delimiter ? 2 : 1;

      int closing_byte_offset = find_markdown_delimiter_byte_offset(
        text,
        byte_length,
        scan_byte_offset + delimiter_byte_length,
        '*',
        delimiter_byte_length
      );

      if (closing_byte_offset > scan_byte_offset + delimiter_byte_length) {
        span_start_byte_offset = scan_byte_offset + delimiter_byte_length;
        span_byte_length = closing_byte_offset - span_start_byte_offset;
        style =
          is_strong_delimiter ? MARKDOWN_INLINE_STRONG : MARKDOWN_INLINE_EMPHASIS;
        next_byte_offset = closing_byte_offset + delimiter_byte_length;
      }

    } else if (
      byte == '[' ||
      (byte == '!' && scan_byte_offset + 1 < byte_length &&
       text[scan_byte_offset + 1] == '[')
    ) {
      int opening_bracket_byte_offset =
        byte == '!' ? scan_byte_offset + 1 : scan_byte_offset;
      int label_end_byte_offset, link_end_byte_offset;

      if (scan_markdown_link(
            text,
            byte_length,
            opening_bracket_byte_offset,
            &label_end_byte_offset,
            &link_end_byte_offset
          )) {
        span_start_byte_offset = opening_bracket_byte_offset + 1;
        span_byte_length =
          label_end_byte_offset - opening_bracket_byte_offset - 1;
        style = MARKDOWN_INLINE_LINK;
        next_byte_offset = link_end_byte_offset;
      }
    }

    if (span_start_byte_offset < 0) {
      scan_byte_offset += vim_utf8_byte_length((uint8_t)byte);
      continue;
    }

    if (scan_byte_offset > plain_start_byte_offset)
      write_span(
        writer_context,
        text + plain_start_byte_offset,
        scan_byte_offset - plain_start_byte_offset,
        MARKDOWN_INLINE_PLAIN
      );

    if (span_byte_length > 0)
      write_span(
        writer_context,
        text + span_start_byte_offset,
        span_byte_length,
        style
      );

    scan_byte_offset = next_byte_offset;
    plain_start_byte_offset = next_byte_offset;
  }

  if (byte_length > plain_start_byte_offset)
    write_span(
      writer_context,
      text + plain_start_byte_offset,
      byte_length - plain_start_byte_offset,
      MARKDOWN_INLINE_PLAIN
    );
}
