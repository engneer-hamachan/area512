#include "core/mode/normal.h"
#include "core/mode/insert.h"
#include "core/register/paste.h"
#include "core/register/repeat.h"
#include "core/register/search.h"
#include "core/render/footer.h"
#include <stddef.h>
#include <string.h>

static int
count_page_lines(Vim *vim) {
  int lines = vim->screen.height - vim->screen.footer_height - 1;

  if (lines < 1)
    return 1;

  return lines;
}

static void
move_cursor_lines(Vim *vim, int count) {
  if (count > 0)
    for (int i = 0; i < count; i++)
      vim_buffer_put_key(BUFFER, VIM_PUT_DOWN);
  else
    for (int i = 0; i < -count; i++)
      vim_buffer_put_key(BUFFER, VIM_PUT_UP);
}

static VimStatus
complete_pending(
  Vim *vim,
  int key,
  const char *character,
  int character_byte_length
) {

  int pending = vim->input.pending;
  vim->input.pending = VIM_PENDING_NONE;

  switch (pending) {
  case VIM_PENDING_REPLACE:
    if (key >= 32 && character && character_byte_length > 0) { // printable ASCII, ' ' (space) and above
      vim_buffer_replace_char(BUFFER, character, character_byte_length);

      memcpy(
        vim->repeat.last_change_character,
        character,
        (size_t)character_byte_length
      );

      vim->repeat.last_change_character_byte_length = character_byte_length;

      remember_last_change(vim, VIM_LAST_CHANGE_REPLACE);

      REDRAW(VIM_REDRAW_CURRENT_LINE);
    }

    break;

  case VIM_PENDING_FIND:
    if (character && character_byte_length > 0) {
      int found;

      if (vim->input.pending_direction > 0)
        found =
          vim_buffer_find_char_forward(
            BUFFER,
            character,
            character_byte_length,
            vim->input.pending_stop_before_match
          );

      else
        found =
          vim_buffer_find_char_backward(
            BUFFER,
            character,
            character_byte_length,
            vim->input.pending_stop_before_match
          );

      if (found) {
        vim_string_set(
          &vim->search.last_find_char,
          character,
          character_byte_length
        );

        vim->search.last_find_direction = vim->input.pending_direction;

        vim->search.last_find_stop_before_match =
          vim->input.pending_stop_before_match;
      }
    }

    break;

  case VIM_PENDING_G:
    if (key == 103) { // 'g'
      if (vim->input.pending_has_count)
        vim_buffer_go_to_line(BUFFER, vim->input.pending_count - 1, 1);
      else
        vim_buffer_move_to_first_line(BUFFER);
    }

    break;

  case VIM_PENDING_OUTDENT:
    if (key == 60) { // '<'
      vim_buffer_outdent_line(BUFFER, INDENT_UNIT, INDENT_UNIT_BYTE_LENGTH);
      remember_last_change(vim, VIM_LAST_CHANGE_OUTDENT);

      REDRAW(VIM_REDRAW_CURRENT_LINE);
    }

    break;

  case VIM_PENDING_INDENT:
    if (key == 62) { // '>'
      vim_buffer_indent_line(BUFFER, INDENT_UNIT, INDENT_UNIT_BYTE_LENGTH);
      remember_last_change(vim, VIM_LAST_CHANGE_INDENT);

      REDRAW(VIM_REDRAW_CURRENT_LINE);
    }

    break;
  case VIM_PENDING_YANK:
    if (key == 121) { // 'y'
      vim_string_set(
        &vim->paste.text,
        BUFFER->lines[BUFFER->cursor_line_index].bytes,
        BUFFER->lines[BUFFER->cursor_line_index].byte_length
      );

      vim->paste.is_line = 1;
    }

    break;
  }

  return VIM_CONTINUE;
}

VimStatus
handle_normal(
  Vim *vim,
  int key,
  const char *character,
  int character_byte_length
) {

  if (vim->input.pending != VIM_PENDING_NONE)
    return complete_pending(vim, key, character, character_byte_length);

  if (key >= 48 && key <= 57) { // '0' to '9'
    if (key == 48 && !vim->input.has_count) { // '0'
      vim_buffer_move_to_line_head(BUFFER);
      return VIM_CONTINUE;
    }

    int count_prefix = 0;
    if (vim->input.has_count)
      count_prefix = vim->input.count;

    vim->input.count = count_prefix * 10 + (key - 48); // '0'
    vim->input.has_count = 1;

    vim_string_clear(&vim->status.command);
    vim_string_append_integer(&vim->status.command, vim->input.count);

    REDRAW(VIM_REDRAW_FOOTER);

    return VIM_CONTINUE;
  }

  int count = 0;
  if (vim->input.has_count)
    count = vim->input.count;

  int had_count = vim->input.has_count;

  vim->input.has_count = 0;
  vim->input.count = 0;

  if (had_count) {
    clear_command(vim);
    REDRAW(VIM_REDRAW_ALL);
  }

  int repeat_count = 1;
  if (had_count)
    repeat_count = count;

  switch (key) {
  case 2: // Ctrl-B (STX)
    move_cursor_lines(vim, -count_page_lines(vim));

    break;

  case 3: // Ctrl-C (ETX)
    vim_string_set(
      &vim->status.command,
      "Type  :q  and press <Enter> to exit",
      35
    );

    REDRAW(VIM_REDRAW_FOOTER);

    break;

  case 21: // Ctrl-U (NAK)
    move_cursor_lines(vim, -(count_page_lines(vim) / 2));

    break;

  case 6: // Ctrl-F (ACK)
    move_cursor_lines(vim, count_page_lines(vim));

    break;

  case 13: // '\r' (CR), Enter
    if (BUFFER->cursor_line_index + 1 < BUFFER->line_count) {
      vim_buffer_put_key(BUFFER, VIM_PUT_DOWN);
      vim_buffer_move_to_line_head(BUFFER);
    }

    break;

  case 18: // Ctrl-R (DC2)
    REDRAW(VIM_REDRAW_ALL);

    break;

  case 4: // Ctrl-D (EOT)
    move_cursor_lines(vim, count_page_lines(vim) / 2);

    break;

  case 36: // '$'
    vim_buffer_move_to_line_end(BUFFER);

    break;

  case 37: // '%'
    vim_buffer_find_matching_pair(BUFFER);

    break;

  case 46: // '.'
    repeat_last_change(vim);

    break;

  case 47: // '/'
    vim->input.mode = VIM_MODE_SEARCH;

    vim_buffer_clear_selection(BUFFER);
    vim_string_set(&vim->status.command, "/", 1);

    REDRAW(VIM_REDRAW_FOOTER);

    break;

  case 58: // ':'
    vim->input.mode = VIM_MODE_COMMAND;

    vim_string_set(&vim->status.command, ":", 1);

    REDRAW(VIM_REDRAW_FOOTER);

    break;

  case 59: // ';'
    repeat_find(vim, 0);

    break;

  case 44: // ','
    repeat_find(vim, 1);

    break;

  case 60: // '<'
    vim->input.pending = VIM_PENDING_OUTDENT;

    break;

  case 62: // '>'
    vim->input.pending = VIM_PENDING_INDENT;

    break;

  case 65: // 'A'
  case 73: // 'I'
  case 97: // 'a'
  case 105: // 'i'
    enter_insert_from_key(vim, key);

    REDRAW(VIM_REDRAW_FOOTER);

    break;

  case 111: // 'o'
  case 79: // 'O'
    enter_insert_from_key(vim, key);

    if (key == 111) // 'o'
      remember_last_change(vim, VIM_LAST_CHANGE_OPEN_BELOW);
    else
      remember_last_change(vim, VIM_LAST_CHANGE_OPEN_ABOVE);

    REDRAW(VIM_REDRAW_ALL);

    break;

  case 66: // 'B'
    for (int i = 0; i < repeat_count; i++)
      vim_buffer_move_to_previous_word_group(BUFFER);

    break;

  case 67: // 'C'
    vim_buffer_delete_to_eol(BUFFER, NULL);
    remember_last_change(vim, VIM_LAST_CHANGE_CHANGE_EOL);
    enter_insert(vim);

    REDRAW(VIM_REDRAW_ALL);

    break;

  case 68: // 'D'
    vim_string_clear(&vim->paste.text);
    vim_buffer_delete_to_eol(BUFFER, &vim->paste.text);

    vim->paste.is_line = 0;

    remember_last_change(vim, VIM_LAST_CHANGE_DELETE_EOL);

    REDRAW(VIM_REDRAW_ALL);

    break;

  case 69: // 'E'
    for (int i = 0; i < repeat_count; i++)
      vim_buffer_move_to_word_group_end(BUFFER);

    break;

  case 71: // 'G'
    if (had_count)
      vim_buffer_go_to_line(BUFFER, count - 1, 1);
    else
      vim_buffer_move_to_last_line(BUFFER);

    break;

  case 74: // 'J'
    if (BUFFER->cursor_line_index + 1 < BUFFER->line_count) {
      vim_buffer_join_next_line(BUFFER);
      remember_last_change(vim, VIM_LAST_CHANGE_JOIN);

      REDRAW(VIM_REDRAW_ALL);
    }

    break;

  case 78: // 'N'
    search_again(vim, 0);

    break;

  case 80: // 'P'
    paste_stored_text(vim, 0);

    vim->repeat.last_change_paste_after_cursor = 0;

    remember_last_change(vim, VIM_LAST_CHANGE_PASTE);

    REDRAW(VIM_REDRAW_ALL);

    break;

  case 83: // 'S'
    clear_current_line_and_enter_insert(vim);
    remember_last_change(vim, VIM_LAST_CHANGE_CHANGE_LINE);

    REDRAW(VIM_REDRAW_ALL);

    break;

  case 86: // 'V'
    vim->input.mode = VIM_MODE_VISUAL_LINE;

    vim_buffer_start_selection(BUFFER, VIM_SELECTION_LINE);
    vim_string_set(&vim->status.command, "Visual Line", 11);

    REDRAW(VIM_REDRAW_FOOTER);

    break;

  case 87: // 'W'
    for (int i = 0; i < repeat_count; i++)
      vim_buffer_move_to_next_word_group(BUFFER);

    break;

  case 94: // '^'
    vim_buffer_move_to_line_first_nonblank(BUFFER);

    break;

  case 98: // 'b'
    for (int i = 0; i < repeat_count; i++)
      vim_buffer_move_to_previous_word(BUFFER);

    break;

  case 100: // 'd'
    vim->input.mode = VIM_MODE_OPERATOR;
    vim->input.current_operator = VIM_OPERATOR_DELETE;

    vim_string_set(&vim->status.command, "d", 1);

    REDRAW(VIM_REDRAW_FOOTER);

    break;

  case 99: // 'c'
    vim->input.mode = VIM_MODE_OPERATOR;
    vim->input.current_operator = VIM_OPERATOR_CHANGE;

    vim_string_set(&vim->status.command, "c", 1);

    REDRAW(VIM_REDRAW_FOOTER);

    break;

  case 101: // 'e'
    for (int i = 0; i < repeat_count; i++)
      vim_buffer_move_to_word_end(BUFFER);

    break;

  case 102: // 'f'
    vim->input.pending = VIM_PENDING_FIND;
    vim->input.pending_direction = 1;
    vim->input.pending_stop_before_match = 0;

    break;

  case 70: // 'F'
    vim->input.pending = VIM_PENDING_FIND;
    vim->input.pending_direction = -1;
    vim->input.pending_stop_before_match = 0;

    break;

  case 103: // 'g'
    vim->input.pending = VIM_PENDING_G;
    vim->input.pending_count = count;
    vim->input.pending_has_count = had_count;

    break;

  case 104: // 'h'
    for (int i = 0; i < repeat_count; i++)
      vim_buffer_move_left(BUFFER);

    break;

  case 106: // 'j'
    for (int i = 0; i < repeat_count; i++)
      vim_buffer_move_down(BUFFER);

    break;

  case 107: // 'k'
    for (int i = 0; i < repeat_count; i++)
      vim_buffer_move_up(BUFFER);

    break;

  case 108: // 'l'
    for (int i = 0; i < repeat_count; i++)
      vim_buffer_move_right(BUFFER);

    break;

  case 110: // 'n'
    search_again(vim, 1);

    break;

  case 112: // 'p'
    paste_stored_text(vim, 1);

    vim->repeat.last_change_paste_after_cursor = 1;

    remember_last_change(vim, VIM_LAST_CHANGE_PASTE);

    REDRAW(VIM_REDRAW_ALL);

    break;

  case 114: // 'r'
    vim->input.pending = VIM_PENDING_REPLACE;

    break;

  case 115: // 's'
    vim_buffer_delete_char(BUFFER);
    remember_last_change(vim, VIM_LAST_CHANGE_SUBSTITUTE);
    enter_insert(vim);

    REDRAW(VIM_REDRAW_ALL);

    break;

  case 116: // 't'
    vim->input.pending = VIM_PENDING_FIND;
    vim->input.pending_direction = 1;
    vim->input.pending_stop_before_match = 1;

    break;

  case 84: // 'T'
    vim->input.pending = VIM_PENDING_FIND;
    vim->input.pending_direction = -1;
    vim->input.pending_stop_before_match = 1;

    break;

  case 117: // 'u'
    break;

  case 118: // 'v'
    vim->input.mode = VIM_MODE_VISUAL;

    vim_buffer_start_selection(BUFFER, VIM_SELECTION_CHAR);
    vim_string_set(&vim->status.command, "Visual", 6);

    REDRAW(VIM_REDRAW_FOOTER);

    break;

  case 119: // 'w'
    for (int i = 0; i < repeat_count; i++)
      vim_buffer_move_to_next_word(BUFFER);

    break;

  case 120: // 'x'
    for (int i = 0; i < repeat_count; i++)
      vim_buffer_delete_char(BUFFER);

    remember_last_change(vim, VIM_LAST_CHANGE_DELETE_CHAR);

    REDRAW(VIM_REDRAW_CURRENT_LINE);

    break;

  case 121: // 'y'
    vim->input.pending = VIM_PENDING_YANK;

    break;

  case 123: // '{'
    vim_buffer_move_to_previous_paragraph(BUFFER);

    break;

  case 125: // '}'
    vim_buffer_move_to_next_paragraph(BUFFER);

    break;

  case 126: // '~'
    vim_buffer_toggle_case(BUFFER);

    remember_last_change(vim, VIM_LAST_CHANGE_TOGGLE_CASE);

    REDRAW(VIM_REDRAW_CURRENT_LINE);

    break;

  default:
    break;
  }

  return VIM_CONTINUE;
}
