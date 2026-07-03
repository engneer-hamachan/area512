#include "core/mode/insert.h"
#include "core/syntax/picoruby/highlight.h"
#include "core/syntax/picoruby/indent.h"
#include <stddef.h>
#include <string.h>

void
enter_insert(Vim *vim) {
  vim->input.mode = VIM_MODE_INSERT;
  vim_string_set(&vim->status.command, "Insert", 6);
}

static void
put_auto_indented_newline(Vim *vim) {
  const char *line = BUFFER->lines[BUFFER->cursor_line_index].bytes;
  int byte_length = BUFFER->lines[BUFFER->cursor_line_index].byte_length;
  int cursor_byte_offset = BUFFER->cursor_byte_offset;

  if (cursor_byte_offset > byte_length)
    cursor_byte_offset = byte_length;

  int space_count = 0;
  while (space_count < cursor_byte_offset && line[space_count] == ' ')
    space_count++;

  const char *previous_line = NULL;
  int previous_byte_length = 0;

  if (BUFFER->cursor_line_index > 0) {
    previous_line = BUFFER->lines[BUFFER->cursor_line_index - 1].bytes;

    previous_byte_length =
      BUFFER->lines[BUFFER->cursor_line_index - 1].byte_length;
  }

  int extra_indent = 0;
  if (editor_is_ruby_filename(vim->filepath.bytes, vim->filepath.byte_length) &&
      editor_auto_indent_should_increase(
        line,
        byte_length,
        previous_line,
        previous_byte_length
      )) {

    extra_indent = INDENT_UNIT_BYTE_LENGTH;
  }

  vim_buffer_put_key(BUFFER, VIM_PUT_ENTER);

  int total_indent = space_count + extra_indent;

  if (total_indent > 0) {
    VimString indent;
    vim_string_init(&indent);

    for (int i = 0; i < total_indent; i++)
      vim_string_append_byte(&indent, ' ');

    vim_buffer_put_string(BUFFER, indent.bytes, indent.byte_length);
    vim_string_free(&indent);
  }
}

static int
matches_keyword(const char *text, int byte_length, const char *keyword) {
  int keyword_byte_length = (int)strlen(keyword);

  return byte_length == keyword_byte_length &&
         memcmp(text, keyword, (size_t)keyword_byte_length) == 0;
}

static int
is_block_outdent_keyword(const char *text, int byte_length) {
  static const char *const keywords[] = {
    "in", "end", "else", "when", "elsif", "rescue", "ensure",
  };
  for (int i = 0; i < (int)(sizeof(keywords) / sizeof(keywords[0])); i++)
    if (matches_keyword(text, byte_length, keywords[i]))
      return 1;
  return 0;
}

static void
maybe_outdent_current_line(Vim *vim) {
  if (!editor_is_ruby_filename(vim->filepath.bytes, vim->filepath.byte_length))
    return;

  VimString *line = &BUFFER->lines[BUFFER->cursor_line_index];

  if (BUFFER->cursor_byte_offset == 0 ||
      BUFFER->cursor_byte_offset > line->byte_length)
    return;

  int start = 0;
  while (start < line->byte_length &&
         (line->bytes[start] == ' ' || line->bytes[start] == '\t'))
    start++;

  int keyword_byte_length = BUFFER->cursor_byte_offset - start;
  if (keyword_byte_length <= 0)
    return;

  if (!is_block_outdent_keyword(line->bytes + start, keyword_byte_length))
    return;

  if (!editor_auto_indent_should_decrease(line->bytes, line->byte_length))
    return;

  vim_buffer_outdent_line(BUFFER, INDENT_UNIT, INDENT_UNIT_BYTE_LENGTH);
}

static void
open_line_below(Vim *vim) {
  vim_buffer_move_to_line_tail(BUFFER);
  put_auto_indented_newline(vim);
}

static void
open_line_above(Vim *vim) {
  if (BUFFER->cursor_line_index == 0) {
    vim_buffer_insert_lines_above(BUFFER, "", 0);
  } else {
    vim_buffer_move_up(BUFFER);
    vim_buffer_move_to_line_tail(BUFFER);

    put_auto_indented_newline(vim);
  }
}

void
enter_insert_from_key(Vim *vim, int key) {
  switch (key) {
  case 65: // 'A'
    vim_buffer_move_to_line_tail(BUFFER);
    break;
  case 73: // 'I'
    vim_buffer_move_to_line_head(BUFFER);
    vim_buffer_move_to_line_first_nonblank(BUFFER);
    break;
  case 97: // 'a'
    vim_buffer_move_right(BUFFER);
    break;
  case 111: // 'o'
    open_line_below(vim);
    break;
  case 79: // 'O'
    open_line_above(vim);
    break;
  }

  enter_insert(vim);
}

void
clear_current_line_and_enter_insert(Vim *vim) {
  vim_buffer_set_line(BUFFER, BUFFER->cursor_line_index, "", 0);
  vim_buffer_move_to_line_head(BUFFER);

  BUFFER->changed = 1;
  vim_buffer_mark_dirty(BUFFER, VIM_DIRTY_STRUCTURE);

  enter_insert(vim);
}

void
handle_insert(
  Vim *vim,
  int key,
  const char *character,
  int character_byte_length
) {
  switch (key) {
  case 8: // '\b' (BS), Backspace
  case 127: // DEL, Backspace
    vim_buffer_put_key(BUFFER, VIM_PUT_BACKSPACE);
    break;
  case 9: // '\t' (HT), Tab
    vim_buffer_put_key(BUFFER, VIM_PUT_TAB);
    break;
  case 10: // '\n' (LF), Enter
  case 13: // '\r' (CR), Enter
    put_auto_indented_newline(vim);
    break;
  default:
    if (key >= 32 && character && character_byte_length > 0) { // printable ASCII, ' ' (space) and above
      vim_buffer_put_string(BUFFER, character, character_byte_length);
      maybe_outdent_current_line(vim);
    }

    break;
  }
}
