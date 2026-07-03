#include "core/mode/operator.h"
#include "core/mode/insert.h"
#include "core/register/repeat.h"
#include "core/render/footer.h"
#include "core/text/utf8.h"

int
apply_operator_motion(Vim *vim, VimOperator operator_kind, int key) {
  if (key != 36 && key != 48 && key != 94 && key != 119 && key != 101)
    return 0;

  int start_line_index = BUFFER->cursor_line_index;
  int start_byte_offset = BUFFER->cursor_byte_offset;
  int inclusive = 1;

  switch (key) {
  case 36:
    vim_buffer_move_to_line_end(BUFFER);

    break;

  case 48:
    vim_buffer_move_to_line_head(BUFFER);

    break;

  case 94:
    vim_buffer_move_to_line_first_nonblank(BUFFER);

    break;

  case 119:
    vim_buffer_move_to_next_word(BUFFER);

    inclusive = 0;

    break;

  case 101:
    vim_buffer_move_to_word_end(BUFFER);

    break;
  }

  int end_line_index = BUFFER->cursor_line_index;
  int end_byte_offset = BUFFER->cursor_byte_offset;

  if (!inclusive) {
    if (start_line_index == end_line_index && end_byte_offset > 0) {
      end_byte_offset = vim_previous_character_byte_offset(
        BUFFER->lines[end_line_index].bytes,
        end_byte_offset
      );
    } else if (end_line_index > start_line_index) {
      end_line_index -= 1;
      end_byte_offset = BUFFER->lines[end_line_index].byte_length - 1;
    }
  }

  vim_string_clear(&vim->paste.text);

  vim_buffer_delete_range(
    BUFFER,
    start_line_index,
    start_byte_offset,
    end_line_index,
    end_byte_offset,
    &vim->paste.text
  );

  vim->paste.is_line = 0;

  if (operator_kind == VIM_OPERATOR_CHANGE)
    enter_insert(vim);

  return 1;
}

void
handle_operator(Vim *vim, int key) {
  VimOperator operator_kind = vim->input.current_operator;
  vim->input.current_operator = VIM_OPERATOR_NONE;
  vim->input.mode = VIM_MODE_NORMAL;

  clear_command(vim);

  int changed = 0;

  switch (key) {
  case 100:
    if (operator_kind == VIM_OPERATOR_DELETE) {
      vim_string_clear(&vim->paste.text);
      vim_buffer_delete_line(BUFFER, &vim->paste.text);

      vim->paste.is_line = 1;
      vim->repeat.last_change_operator = VIM_OPERATOR_DELETE;
      vim->repeat.last_change_motion = key;

      remember_last_change(vim, VIM_LAST_CHANGE_OPERATOR);

      changed = 1;
    }

    break;

  case 99:
    if (operator_kind == VIM_OPERATOR_CHANGE) {
      clear_current_line_and_enter_insert(vim);
      remember_last_change(vim, VIM_LAST_CHANGE_CHANGE_LINE);
      changed = 1;
    }

    break;

  default:
    changed = apply_operator_motion(vim, operator_kind, key);

    if (changed) {
      vim->repeat.last_change_operator = operator_kind;
      vim->repeat.last_change_motion = key;

      remember_last_change(vim, VIM_LAST_CHANGE_OPERATOR);
    }

    break;
  }

  if (changed)
    REDRAW(VIM_REDRAW_ALL);
  else
    REDRAW(VIM_REDRAW_FOOTER);
}
