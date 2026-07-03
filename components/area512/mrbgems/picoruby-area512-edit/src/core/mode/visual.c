#include "core/mode/visual.h"
#include "core/render/footer.h"

void
handle_visual(Vim *vim, int key) {
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

  case 66:
    vim_buffer_move_to_previous_word_group(BUFFER);

    break;

  case 69:
    vim_buffer_move_to_word_group_end(BUFFER);

    break;

  case 71:
    vim_buffer_go_to_line(BUFFER, BUFFER->line_count - 1, 1);

    break;

  case 87:
    vim_buffer_move_to_next_word_group(BUFFER);

    break;

  case 98:
    vim_buffer_move_to_previous_word(BUFFER);

    break;

  case 101:
    vim_buffer_move_to_word_end(BUFFER);

    break;

  case 103:
    vim_buffer_move_home(BUFFER);

    break;

  case 104:
    vim_buffer_put_key(BUFFER, VIM_PUT_LEFT);

    break;

  case 106:
    vim_buffer_put_key(BUFFER, VIM_PUT_DOWN);

    break;

  case 107:
    vim_buffer_put_key(BUFFER, VIM_PUT_UP);

    break;

  case 108:
    vim_buffer_put_key(BUFFER, VIM_PUT_RIGHT);

    break;

  case 119:
    vim_buffer_move_to_next_word(BUFFER);

    break;

  case 121: {
    int start_line_index, start_byte_offset, end_line_index, end_byte_offset;

    int has_selection =
      vim_buffer_selection_range(
        BUFFER,
        &start_line_index,
        &start_byte_offset,
        &end_line_index,
        &end_byte_offset
      );

    vim_string_clear(&vim->paste.text);
    vim_buffer_copy_selected_text(BUFFER, &vim->paste.text);

    vim->paste.is_line = 0;

    if (BUFFER->selection_mode == VIM_SELECTION_LINE)
      vim->paste.is_line = 1;
    if (has_selection)
      vim_buffer_move_to(BUFFER, start_byte_offset, start_line_index);

    vim_buffer_clear_selection(BUFFER);

    vim->input.mode = VIM_MODE_NORMAL;

    clear_command(vim);

    REDRAW(VIM_REDRAW_ALL);

    return;
  }
  case 100:
  case 120: {
    vim->paste.is_line = 0;

    if (BUFFER->selection_mode == VIM_SELECTION_LINE)
      vim->paste.is_line = 1;

    vim_string_clear(&vim->paste.text);
    vim_buffer_delete_selected_text(BUFFER, &vim->paste.text);
    vim_buffer_clear_selection(BUFFER);

    vim->input.mode = VIM_MODE_NORMAL;

    clear_command(vim);

    REDRAW(VIM_REDRAW_ALL);

    return;
  }
  case 118:
    if (vim->input.mode == VIM_MODE_VISUAL) {
      vim_buffer_clear_selection(BUFFER);

      vim->input.mode = VIM_MODE_NORMAL;

      clear_command(vim);

      REDRAW(VIM_REDRAW_FOOTER);

      return;
    }

    vim_buffer_clear_selection(BUFFER);
    vim_buffer_start_selection(BUFFER, VIM_SELECTION_CHAR);

    vim->input.mode = VIM_MODE_VISUAL;

    vim_string_set(&vim->status.command, "Visual", 6);

    REDRAW(VIM_REDRAW_FOOTER);

    return;

  case 86:
    if (vim->input.mode == VIM_MODE_VISUAL_LINE) {
      vim_buffer_clear_selection(BUFFER);

      vim->input.mode = VIM_MODE_NORMAL;

      clear_command(vim);

      REDRAW(VIM_REDRAW_FOOTER);

      return;
    }

    vim_buffer_clear_selection(BUFFER);
    vim_buffer_start_selection(BUFFER, VIM_SELECTION_LINE);

    vim->input.mode = VIM_MODE_VISUAL_LINE;

    vim_string_set(&vim->status.command, "Visual Line", 11);

    REDRAW(VIM_REDRAW_FOOTER);

    return;

  default:
    break;
  }

  if (vim->input.mode == VIM_MODE_VISUAL ||
      vim->input.mode == VIM_MODE_VISUAL_LINE)

    REDRAW(VIM_REDRAW_ALL);
}
