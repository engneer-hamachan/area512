#include "core/render/footer.h"
#include <string.h>

#define FOOTER_FOREGROUND 0xFFFFFF
#define FOOTER_BACKGROUND 0x4E4E4E
#define FOOTER_MESSAGE 0xFFFFFF

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

  const char *name;
  int name_byte_length;

  if (vim->filepath.byte_length > 0) {
    name = vim->filepath.bytes;
    name_byte_length = vim->filepath.byte_length;
  } else {
    name = "[No Name]";
    name_byte_length = 9;
  }

  canvas->clear_row(canvas->context);

  canvas->draw_row_text(
    canvas->context,
    0,
    " ",
    1,
    FOOTER_FOREGROUND,
    FOOTER_BACKGROUND,
    0
  );

  int max_name_byte_length = width - 1;

  if (max_name_byte_length < 0)
    max_name_byte_length = 0;

  int shown_byte_length = max_name_byte_length;

  if (name_byte_length < max_name_byte_length)
    shown_byte_length = name_byte_length;

  canvas->draw_row_text(
    canvas->context,
    1,
    name,
    shown_byte_length,
    FOOTER_FOREGROUND,
    FOOTER_BACKGROUND,
    0
  );

  char padding[64];
  int padding_byte_length = max_name_byte_length - shown_byte_length;

  if (padding_byte_length > (int)sizeof(padding))
    padding_byte_length = (int)sizeof(padding);

  for (int i = 0; i < padding_byte_length; i++)
    padding[i] = ' ';

  if (padding_byte_length > 0)
    canvas->draw_row_text(
      canvas->context,
      1 + shown_byte_length,
      padding,
      padding_byte_length,
      FOOTER_FOREGROUND,
      FOOTER_BACKGROUND,
      0
    );

  canvas->push_row(canvas->context, footer_row);

  canvas->clear_row(canvas->context);

  if (vim->status.has_message) {
    canvas->draw_row_text(
      canvas->context,
      0,
      vim->status.message.bytes,
      vim->status.message.byte_length,
      FOOTER_MESSAGE,
      0,
      0
    );

    vim->status.has_message = 0;

    vim_string_clear(&vim->status.message);

  } else {
    canvas->draw_row_text(
      canvas->context,
      0,
      vim->status.command.bytes,
      vim->status.command.byte_length,
      0,
      0,
      0
    );
  }

  canvas->push_row(canvas->context, footer_row + 1);

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
