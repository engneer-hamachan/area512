#include "core/editor.h"
#include "area512_hal.h"
#include "core/mode/command.h"
#include "core/mode/insert.h"
#include "core/mode/normal.h"
#include "core/mode/operator.h"
#include "core/mode/visual.h"
#include "core/render/footer.h"
#include "core/syntax/syntax.h"
#include "core/text/utf8.h"
#include <string.h>

#define VIM_DIAGNOSTIC_FOREGROUND 0xD16969

static void
handle_escape_arrow(Vim *vim, int letter) {
  switch (letter) {
  case 'A':
    vim_buffer_put_key(BUFFER, VIM_PUT_UP);
    break;
  case 'B':
    vim_buffer_put_key(BUFFER, VIM_PUT_DOWN);
    break;
  case 'C':
    vim_buffer_put_key(BUFFER, VIM_PUT_RIGHT);
    break;
  case 'D':
    vim_buffer_put_key(BUFFER, VIM_PUT_LEFT);
    break;
  }
}

void
vim_init(Vim *vim, int width, int height) {
  memset(vim, 0, sizeof(*vim));
  vim_screen_init(&vim->screen, width, height);
  vim->screen.footer_height = 1;

  vim->screen.syntax = VIM_SYNTAX_NONE;
  vim->screen.footer = draw_vim_footer;
  vim->screen.footer_context = vim;
  vim->screen.draw_cursor = draw_vim_cursor;
  vim->screen.draw_cursor_context = vim;
  vim->screen.diagnostic = vim_draw_diagnostics;
  vim->screen.diagnostic_context = vim;
  vim->input.mode = VIM_MODE_NORMAL;
  vim->input.current_operator = VIM_OPERATOR_NONE;
  vim->input.pending = VIM_PENDING_NONE;
  vim_string_init(&vim->status.command);
  vim_string_init(&vim->status.message);
  vim_string_init(&vim->paste.text);
  vim_string_init(&vim->filepath);
  vim_string_init(&vim->search.last_search);
  vim_string_init(&vim->search.last_find_char);
  vim->repeat.last_change = VIM_LAST_CHANGE_NONE;
}

void
vim_free(Vim *vim) {
  vim_screen_free(&vim->screen);
  vim_string_free(&vim->status.command);
  vim_string_free(&vim->status.message);
  vim_string_free(&vim->paste.text);
  vim_string_free(&vim->filepath);
  vim_string_free(&vim->search.last_search);
  vim_string_free(&vim->search.last_find_char);
}

void
vim_clear_diagnostics(Vim *vim) {
  vim->diagnostics.count = 0;
}

void
vim_draw_diagnostics(
  void *vim_context,
  VimCanvas *canvas,
  int line_start_buffer_byte_offset,
  int segment_draw_column,
  const char *line,
  int line_byte_length,
  int segment_start_line_column,
  int segment_column_capacity
) {

  Vim *vim = (Vim *)vim_context;

  int segment_start_line_byte_offset =
    vim_column_to_byte(
      line,
      line_byte_length,
      segment_start_line_column
    );

  int segment_end_line_byte_offset =
    vim_column_to_byte(
      line,
      line_byte_length,
      segment_start_line_column + segment_column_capacity
    );

  int segment_start_buffer_byte_offset =
    line_start_buffer_byte_offset + segment_start_line_byte_offset;

  int segment_end_buffer_byte_offset =
    line_start_buffer_byte_offset + segment_end_line_byte_offset;

  const char *segment = line + segment_start_line_byte_offset;

  for (int index = 0; index < vim->diagnostics.count; index++) {
    const VimDiagnostic *diagnostic = &vim->diagnostics.items[index];

    int diagnostic_start_buffer_byte_offset = diagnostic->start_byte_offset;
    int diagnostic_end_buffer_byte_offset = diagnostic->end_byte_offset;

    if (
      diagnostic_start_buffer_byte_offset <
      segment_start_buffer_byte_offset
    ) {

      diagnostic_start_buffer_byte_offset =
        segment_start_buffer_byte_offset;
    }

    if (
      diagnostic_end_buffer_byte_offset >
      segment_end_buffer_byte_offset
    ) {

      diagnostic_end_buffer_byte_offset =
        segment_end_buffer_byte_offset;
    }

    if (
      diagnostic_start_buffer_byte_offset >=
      diagnostic_end_buffer_byte_offset
    ) {

      continue;
    }

    int diagnostic_start_line_byte_offset =
      diagnostic_start_buffer_byte_offset - line_start_buffer_byte_offset;

    int diagnostic_end_line_byte_offset =
      diagnostic_end_buffer_byte_offset - line_start_buffer_byte_offset;

    int diagnostic_start_segment_byte_offset =
      diagnostic_start_line_byte_offset - segment_start_line_byte_offset;

    int diagnostic_start_segment_column =
      vim_display_width(
        segment,
        diagnostic_start_segment_byte_offset
      );

    int diagnostic_draw_column =
      segment_draw_column + diagnostic_start_segment_column;

    const char *diagnostic_text =
      line + diagnostic_start_line_byte_offset;

    int diagnostic_byte_length =
      diagnostic_end_line_byte_offset - diagnostic_start_line_byte_offset;

    int diagnostic_column_count =
      vim_display_width(
        diagnostic_text,
        diagnostic_byte_length
      );

    canvas->draw_row_text(
      canvas->context,
      diagnostic_draw_column,
      diagnostic_text,
      diagnostic_byte_length,
      VIM_DIAGNOSTIC_FOREGROUND,
      area512_theme_background_color(),
      0
    );

    if (canvas->draw_row_underline)
      canvas->draw_row_underline(
        canvas->context,
        diagnostic_draw_column,
        diagnostic_column_count,
        VIM_DIAGNOSTIC_FOREGROUND
      );
  }
}

void
vim_set_filepath(Vim *vim, const char *text, int byte_length) {
  vim_string_set(&vim->filepath, text, byte_length);
  vim->screen.syntax = editor_syntax_for_filename(text, byte_length);
}

void
vim_load_text(Vim *vim, const char *text, int byte_length) {
  vim_clear_diagnostics(vim);
  vim_buffer_load_text(BUFFER, text, byte_length);

  vim_buffer_move_home(BUFFER);
}

void
vim_write_content(Vim *vim, VimString *content) {
  vim_string_clear(content);
  for (int i = 0; i < BUFFER->line_count; i++) {
    if (i)
      vim_string_append_byte(content, '\n');
    vim_string_append(
      content,
      BUFFER->lines[i].bytes,
      BUFFER->lines[i].byte_length
    );
  }
  if (BUFFER->line_count > 0)
    vim_string_append_byte(content, '\n');
}

void
vim_handle_after_save(Vim *vim, int saved) {
  if (saved) {
    VimString message;
    vim_string_init(&message);
    vim_string_append_c_string(&message, "File saved: ");
    vim_string_append(&message, vim->filepath.bytes, vim->filepath.byte_length);
    show_message(vim, message.bytes, message.byte_length);
    vim_string_free(&message);
  } else {
    show_message_c_string(vim, "Failed to save");
  }
  REDRAW(VIM_REDRAW_ALL);
}

VimStatus
vim_handle_key(
  Vim *vim,
  int first_byte,
  const char *character,
  int character_byte_length
) {
  switch (vim->input.mode) {
  case VIM_MODE_NORMAL:
    return handle_normal(vim, first_byte, character, character_byte_length);
  case VIM_MODE_OPERATOR:
    handle_operator(vim, first_byte);
    return VIM_CONTINUE;
  case VIM_MODE_COMMAND:
  case VIM_MODE_SEARCH:
    return handle_command_mode(vim, first_byte);
  case VIM_MODE_INSERT:
    handle_insert(vim, first_byte, character, character_byte_length);
    return VIM_CONTINUE;
  case VIM_MODE_VISUAL:
  case VIM_MODE_VISUAL_LINE:
    handle_visual(vim, first_byte);
    return VIM_CONTINUE;
  default:
    return VIM_CONTINUE;
  }
}

VimStatus
vim_handle_esc(Vim *vim, const char *sequence, int byte_length) {
  int letter = 0;
  if (byte_length >= 2 && sequence[0] == '[')
    letter = sequence[1];
  switch (vim->input.mode) {
  case VIM_MODE_NORMAL:
    vim->input.pending = VIM_PENDING_NONE;
    if (letter)
      handle_escape_arrow(vim, letter);
    break;
  case VIM_MODE_INSERT:
    if (letter) {
      handle_escape_arrow(vim, letter);
    } else {
      clear_command(vim);
      vim->input.mode = VIM_MODE_NORMAL;
      if (BUFFER->cursor_byte_offset > 0)
        vim_buffer_put_key(BUFFER, VIM_PUT_LEFT);
      REDRAW(VIM_REDRAW_FOOTER);
    }
    break;
  case VIM_MODE_COMMAND:
  case VIM_MODE_SEARCH:
    if (!letter) {
      clear_command(vim);
      vim->input.mode = VIM_MODE_NORMAL;
    }
    REDRAW(VIM_REDRAW_FOOTER);
    break;
  case VIM_MODE_VISUAL:
  case VIM_MODE_VISUAL_LINE:
    if (letter) {
      handle_escape_arrow(vim, letter);
      REDRAW(VIM_REDRAW_ALL);
    } else {
      vim_buffer_clear_selection(BUFFER);
      vim->input.mode = VIM_MODE_NORMAL;
      clear_command(vim);
      REDRAW(VIM_REDRAW_FOOTER);
    }
    break;
  default:
    break;
  }
  return VIM_CONTINUE;
}
