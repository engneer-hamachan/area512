#include "core/mode/insert.h"
#include "micropython_ti_suggest.h"
#include "picoruby_ti_suggest.h"
#include "core/complete/complete_popup.h"
#include "core/syntax/picoruby/indent.h"
#include "core/syntax/python/indent.h"
#include "core/syntax/syntax.h"
#include "core/ti/loader.h"
#include "core/ti/parse_message.h"
#include <stddef.h>
#include <string.h>

static int
auto_indent_should_increase(
  VimSyntax syntax,
  const char *line,
  int line_byte_length,
  const char *previous_line,
  int previous_line_byte_length
) {

  switch (syntax) {
  case VIM_SYNTAX_RUBY:
    return editor_auto_indent_should_increase(
      line,
      line_byte_length,
      previous_line,
      previous_line_byte_length
    );

  case VIM_SYNTAX_PYTHON:
    return editor_python_auto_indent_should_increase(
      line,
      line_byte_length,
      previous_line,
      previous_line_byte_length
    );

  default:
    return 0;
  }
}

static int
auto_indent_should_decrease(
  VimSyntax syntax,
  const char *line,
  int line_byte_length
) {

  switch (syntax) {
  case VIM_SYNTAX_RUBY:
    return editor_auto_indent_should_decrease(line, line_byte_length);

  case VIM_SYNTAX_PYTHON:
    return editor_python_auto_indent_should_decrease(line, line_byte_length);

  default:
    return 0;
  }
}

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

  if (
    auto_indent_should_increase(
      vim->screen.syntax,
      line,
      cursor_byte_offset,
      previous_line,
      previous_byte_length
    )
  ) {

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
    "in",
    "end",
    "else",
    "when",
    "elsif",
    "rescue",
    "ensure",
  };
  for (int i = 0; i < (int)(sizeof(keywords) / sizeof(keywords[0])); i++)
    if (matches_keyword(text, byte_length, keywords[i]))
      return 1;
  return 0;
}

static int
is_python_block_outdent_keyword(const char *keyword, int keyword_byte_length) {
  return matches_keyword(keyword, keyword_byte_length, "elif") ||
         matches_keyword(keyword, keyword_byte_length, "else") ||
         matches_keyword(keyword, keyword_byte_length, "except") ||
         matches_keyword(keyword, keyword_byte_length, "finally") ||
         matches_keyword(keyword, keyword_byte_length, "case");
}

static void
maybe_outdent_current_line(Vim *vim) {
  VimSyntax syntax = vim->screen.syntax;

  if (syntax == VIM_SYNTAX_NONE)
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

  if (syntax == VIM_SYNTAX_RUBY) {
    if (!is_block_outdent_keyword(line->bytes + start, keyword_byte_length))
      return;

  } else if (syntax == VIM_SYNTAX_PYTHON) {
    if (
      !is_python_block_outdent_keyword(
        line->bytes + start,
        keyword_byte_length
      )
    ) {
      return;
    }
  }

  if (!auto_indent_should_decrease(syntax, line->bytes, line->byte_length))
    return;

  vim_buffer_outdent_line(BUFFER, INDENT_UNIT, INDENT_UNIT_BYTE_LENGTH);
}

static int
calculate_completion_cursor_offset(Vim *vim) {
  int offset = BUFFER->cursor_byte_offset;

  for (int index = 0; index < BUFFER->cursor_line_index; index++)
    offset += BUFFER->lines[index].byte_length + 1;

  return offset;
}

static int
narrows_completion(int key, const char *character, int character_byte_length) {
  if (key == 8 || key == 127)
    return 1;

  if (character_byte_length != 1)
    return 0;

  uint8_t byte = (uint8_t)character[0];

  return (byte >= 'a' && byte <= 'z') || (byte >= 'A' && byte <= 'Z') ||
         (byte >= '0' && byte <= '9') || byte == '_' || byte == '!' ||
         byte == '?';
}

static int
is_numeric_literal_before(const VimString *line, int byte_offset) {
  int digit_start_offset = byte_offset;

  while (
    digit_start_offset > 0 &&
    line->bytes[digit_start_offset - 1] >= '0' &&
    line->bytes[digit_start_offset - 1] <= '9'
  ) {

    digit_start_offset--;
  }

  if (digit_start_offset == byte_offset)
    return 0;

  if (digit_start_offset == 0)
    return 1;

  char preceding_byte = line->bytes[digit_start_offset - 1];

  return !(
    (preceding_byte >= 'a' && preceding_byte <= 'z') ||
    (preceding_byte >= 'A' && preceding_byte <= 'Z') ||
    preceding_byte == '_'
  );
}

static void
start_completion(Vim *vim) {
  int is_ruby =
    editor_is_ruby_filename(vim->filepath.bytes, vim->filepath.byte_length);

  int is_python =
    editor_is_python_filename(vim->filepath.bytes, vim->filepath.byte_length);

  if (!is_ruby && !is_python) {
    return;
  }

  for (;;) {
    VimString content;
    vim_string_init(&content);
    vim_write_content(vim, &content);

    TiLoadedSources loaded_sources;

    if (!load_ti_sources(vim, &content, &loaded_sources)) {
      vim_string_free(&content);
      return;
    }

    TiSuggestionList suggestions;

    int cursor_offset = calculate_completion_cursor_offset(vim);

    int suggestion_count;
    if (is_python) {
      suggestion_count =
        micropython_ti_fill_suggestions_at_cursor(
          &loaded_sources.list,
          cursor_offset,
          &suggestions
        );
    } else {
      suggestion_count =
        ti_fill_suggestions_at_cursor(
          &loaded_sources.list,
          cursor_offset,
          &suggestions
        );
    }

    if (suggestion_count <= 0) {
      if (is_python)
        show_ti_parse_cancelled_message(vim, &loaded_sources);

      free_ti_sources(&loaded_sources);
      return;
    }

    int next_key = 0;
    char next_character[4] = {0};
    int next_character_byte_length = 0;

    int redispatch =
      show_complete_popup(
        vim,
        &suggestions,
        &next_key,
        next_character,
        &next_character_byte_length
      );

    free_ti_sources(&loaded_sources);

    if (!redispatch)
      return;

    handle_insert(vim, next_key, next_character, next_character_byte_length);

    if (!narrows_completion(
          next_key,
          next_character,
          next_character_byte_length
        )) {

      return;
    }

    if (vim->active_canvas)
      vim_screen_refresh_if_needed(&vim->screen, vim->active_canvas);
  }
}

static void
move_right_in_line(VimBuffer *buffer) {
  if (buffer->cursor_byte_offset < vim_buffer_current_line_byte_length(buffer))
    vim_buffer_move_right(buffer);
  else
    vim_buffer_move_to_line_tail(buffer);
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
    move_right_in_line(BUFFER);
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
  case 8:   // '\b' (BS), Backspace
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
  case 14:
    start_completion(vim);
    break;
  default:
    // printable ASCII, ' ' (space) and above
    if (key >= 32 && character && character_byte_length > 0) {
      vim_buffer_put_string(BUFFER, character, character_byte_length);
      maybe_outdent_current_line(vim);
    }

    break;
  }
}
