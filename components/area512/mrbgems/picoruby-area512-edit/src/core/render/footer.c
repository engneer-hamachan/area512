#include "core/render/footer.h"
#include <string.h>

#define FOOTER_FOREGROUND 0xFFFFFF
#define FOOTER_BACKGROUND 0x4E4E4E

void
show_message(Vim *vim, const char *text, int byte_length) {
  vim_string_set(&vim->status.message, text, byte_length);
  vim->status.has_message = 1;
}

void
show_message_c_string(Vim *vim, const char *text) {
  show_message(vim, text, (int)strlen(text));
}

void
clear_command(Vim *vim) {
  vim_string_clear(&vim->status.command);
}

void
draw_vim_footer(void *vim_context, VimCanvas *canvas) {
  Vim *vim = (Vim *)vim_context;
  int width = vim->screen.width;
  int footer_row = vim->screen.height - vim->screen.footer_height;

  const char *text;
  int text_byte_length;
  int column = 0;

  if (vim->status.has_message) {
    text = vim->status.message.bytes;
    text_byte_length = vim->status.message.byte_length;
  } else if (vim->status.command.byte_length > 0) {
    text = vim->status.command.bytes;
    text_byte_length = vim->status.command.byte_length;
  } else if (vim->filepath.byte_length > 0) {
    text = vim->filepath.bytes;
    text_byte_length = vim->filepath.byte_length;
    column = 1;
  } else {
    text = "[No Name]";
    text_byte_length = 9;
    column = 1;
  }

  int max_byte_length = width - column;
  if (max_byte_length < 0)
    max_byte_length = 0;
  if (text_byte_length > max_byte_length)
    text_byte_length = max_byte_length;

  canvas->clear_row(canvas->context);
  canvas->fill_row_span(canvas->context, 0, width, FOOTER_BACKGROUND);
  canvas->draw_row_text(
    canvas->context,
    column,
    text,
    text_byte_length,
    FOOTER_FOREGROUND,
    FOOTER_BACKGROUND,
    0
  );
  canvas->push_row(canvas->context, footer_row);

  if (vim->status.has_message) {
    vim->status.has_message = 0;
    vim_string_clear(&vim->status.message);
  }

  if (vim->input.mode == VIM_MODE_COMMAND || vim->input.mode == VIM_MODE_SEARCH)
    canvas->draw_cursor(
      canvas->context,
      vim->status.command.byte_length,
      vim->screen.height - 1,
      1
    );
}

void
draw_vim_cursor(void *vim_context, VimCanvas *canvas) {
  Vim *vim = (Vim *)vim_context;

  if (vim->input.mode == VIM_MODE_COMMAND || vim->input.mode == VIM_MODE_SEARCH)
    return;

  vim_screen_calculate_cursor(&vim->screen);

  canvas->draw_cursor(
    canvas->context,
    vim->screen.visual_cursor_column + VIM_GUTTER_WIDTH,
    vim->screen.visual_cursor_row,
    1
  );
}
