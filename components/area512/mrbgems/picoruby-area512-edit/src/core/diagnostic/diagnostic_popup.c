#include "core/diagnostic/diagnostic_popup.h"
#include "area512_hal.h"
#include "core/text/utf8.h"
#include <string.h>

#define DIAGNOSTIC_POPUP_FOREGROUND 0xD16969
#define DIAGNOSTIC_POPUP_TEXT_START_COLUMN 1

static int
diagnostic_popup_text_width(Vim *vim) {
  int width =
    vim->screen.width - DIAGNOSTIC_POPUP_TEXT_START_COLUMN - 1;

  return width < 1 ? 1 : width;
}

static int
diagnostic_popup_row_count(
  Vim *vim,
  const char *message,
  int message_byte_length
) {

  int text_width = diagnostic_popup_text_width(vim);
  int message_width = vim_display_width(message, message_byte_length);
  int row_count = (message_width + text_width - 1) / text_width;

  int maximum_row_count =
    vim->screen.height - vim->screen.footer_height;

  if (row_count < 1)
    row_count = 1;

  if (row_count > maximum_row_count)
    row_count = maximum_row_count;

  return row_count;
}

static void
draw_diagnostic_popup(Vim *vim, const char *message) {
  VimCanvas *canvas = vim->active_canvas;

  int message_byte_length = (int)strlen(message);
  int text_width = diagnostic_popup_text_width(vim);
  int row_count = diagnostic_popup_row_count(vim, message, message_byte_length);
  int screen_row = vim->screen.height - vim->screen.footer_height - row_count;
  int message_byte_offset = 0;

  for (int row_index = 0; row_index < row_count; row_index++) {
    int remaining_byte_length = message_byte_length - message_byte_offset;

    int row_byte_length =
      vim_column_to_byte(
        message + message_byte_offset,
        remaining_byte_length,
        text_width
      );

    if (row_byte_length == 0 && remaining_byte_length > 0)
      row_byte_length =
        vim_utf8_byte_length((uint8_t)message[message_byte_offset]);

    canvas->clear_row(canvas->context);

    canvas->fill_row_span(
      canvas->context,
      0,
      vim->screen.width,
      area512_theme_box_color()
    );

    canvas->draw_row_text(
      canvas->context,
      DIAGNOSTIC_POPUP_TEXT_START_COLUMN,
      message + message_byte_offset,
      row_byte_length,
      DIAGNOSTIC_POPUP_FOREGROUND,
      area512_theme_box_color(),
      0
    );

    canvas->push_row(canvas->context, screen_row + row_index);
    message_byte_offset += row_byte_length;
  }
}

void
show_diagnostic_popup(Vim *vim, const char *message) {
  if (!vim || !vim->active_canvas || !message)
    return;

  draw_diagnostic_popup(vim, message);

  while (area512_console_getch_block() < 0) {
  }

  REDRAW(VIM_REDRAW_ALL);
}
