#include "core/register/repeat.h"
#include "core/mode/insert.h"
#include "core/mode/operator.h"
#include "core/register/paste.h"

#include <stddef.h>

void
remember_last_change(Vim *vim, VimLastChange change) {
  vim->repeat.last_change = change;
}

void
repeat_last_change(Vim *vim) {
  switch (vim->repeat.last_change) {
  case VIM_LAST_CHANGE_NONE:
    break;

  case VIM_LAST_CHANGE_DELETE_CHAR:
    vim_buffer_delete_char(BUFFER);

    REDRAW(VIM_REDRAW_CURRENT_LINE);

    break;

  case VIM_LAST_CHANGE_REPLACE:
    vim_buffer_replace_char(
      BUFFER,
      vim->repeat.last_change_character,
      vim->repeat.last_change_character_byte_length
    );

    REDRAW(VIM_REDRAW_CURRENT_LINE);

    break;

  case VIM_LAST_CHANGE_PASTE:
    paste_stored_text(vim, vim->repeat.last_change_paste_after_cursor);

    REDRAW(VIM_REDRAW_ALL);

    break;

  case VIM_LAST_CHANGE_DELETE_EOL:
    vim_string_clear(&vim->paste.text);
    vim_buffer_delete_to_eol(BUFFER, &vim->paste.text);

    vim->paste.is_line = 0;

    REDRAW(VIM_REDRAW_ALL);

    break;

  case VIM_LAST_CHANGE_CHANGE_EOL:
    vim_buffer_delete_to_eol(BUFFER, NULL);
    enter_insert(vim);

    REDRAW(VIM_REDRAW_ALL);

    break;

  case VIM_LAST_CHANGE_JOIN:
    if (BUFFER->cursor_line_index + 1 < BUFFER->line_count) {
      vim_buffer_join_next_line(BUFFER);
      REDRAW(VIM_REDRAW_ALL);
    }

    break;

  case VIM_LAST_CHANGE_INDENT:
    vim_buffer_indent_line(BUFFER, INDENT_UNIT, INDENT_UNIT_BYTE_LENGTH);

    REDRAW(VIM_REDRAW_CURRENT_LINE);

    break;

  case VIM_LAST_CHANGE_OUTDENT:
    vim_buffer_outdent_line(BUFFER, INDENT_UNIT, INDENT_UNIT_BYTE_LENGTH);

    REDRAW(VIM_REDRAW_CURRENT_LINE);

    break;

  case VIM_LAST_CHANGE_TOGGLE_CASE:
    vim_buffer_toggle_case(BUFFER);

    REDRAW(VIM_REDRAW_CURRENT_LINE);

    break;

  case VIM_LAST_CHANGE_CHANGE_LINE:
    clear_current_line_and_enter_insert(vim);

    REDRAW(VIM_REDRAW_ALL);

    break;

  case VIM_LAST_CHANGE_OPERATOR:
    apply_operator_motion(
      vim,
      vim->repeat.last_change_operator,
      vim->repeat.last_change_motion
    );

    REDRAW(VIM_REDRAW_ALL);

    break;

  case VIM_LAST_CHANGE_OPEN_BELOW:
    enter_insert_from_key(vim, 111);

    REDRAW(VIM_REDRAW_ALL);

    break;

  case VIM_LAST_CHANGE_OPEN_ABOVE:
    enter_insert_from_key(vim, 79);

    REDRAW(VIM_REDRAW_ALL);

    break;

  case VIM_LAST_CHANGE_SUBSTITUTE:
    vim_buffer_delete_char(BUFFER);
    enter_insert(vim);

    REDRAW(VIM_REDRAW_ALL);

    break;
  }
}
