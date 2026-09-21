#include "core/render/signature.h"
#include "area512_hal.h"
#include "core/text/utf8.h"
#include <string.h>

#define SIGNATURE_TEXT_START_COLUMN 1

int
minimum_signature_value(int first, int second) {
  return first < second ? first : second;
}

static int
signature_text_width(Vim *vim) {
  int width = vim->screen.width - SIGNATURE_TEXT_START_COLUMN - 1;

  return width < 1 ? 1 : width;
}

static int
signature_class_name_width(const char *class_name) {
  if (!class_name)
    return 0;

  return vim_display_width(class_name, (int)strlen(class_name));
}

static int
can_draw_signature_class_name(
  Vim *vim,
  int signature_width,
  int class_name_width
) {

  return class_name_width > 0 &&
         signature_width + 1 + class_name_width <= signature_text_width(vim);
}

int
count_signature_rows(
  const char *signature,
  int signature_byte_length
) {

  int signature_row_count = 1;

  for (
    int signature_row_start_byte_offset = 0;
    signature_row_start_byte_offset < signature_byte_length;
    signature_row_start_byte_offset++
  ) {

    if (signature[signature_row_start_byte_offset] == '\n')
      signature_row_count++;
  }

  return signature_row_count;
}

int
calculate_maximum_signature_horizontal_scroll_column_count(
  Vim *vim,
  const char *signature,
  int signature_byte_length,
  int signature_row_count
) {

  int available_signature_column_count = signature_text_width(vim);
  int maximum_popup_horizontal_scroll_column_count = 0;
  int signature_row_start_byte_offset = 0;
  int scanned_signature_row_count = 0;

  while (
    signature_row_start_byte_offset <= signature_byte_length &&
    scanned_signature_row_count < signature_row_count
  ) {

    int signature_row_end_byte_offset = signature_row_start_byte_offset;

    while (
      signature_row_end_byte_offset < signature_byte_length &&
      signature[signature_row_end_byte_offset] != '\n'
    ) {

      signature_row_end_byte_offset++;
    }

    int signature_row_byte_length =
      signature_row_end_byte_offset - signature_row_start_byte_offset;

    int signature_row_column_count =
      vim_display_width(
        signature + signature_row_start_byte_offset,
        signature_row_byte_length
      );

    int maximum_signature_horizontal_scroll_column_count =
      signature_row_column_count - available_signature_column_count;

    if (
      maximum_signature_horizontal_scroll_column_count >
      maximum_popup_horizontal_scroll_column_count
    ) {

      maximum_popup_horizontal_scroll_column_count =
        maximum_signature_horizontal_scroll_column_count;
    }

    if (signature_row_end_byte_offset == signature_byte_length)
      break;

    signature_row_start_byte_offset = signature_row_end_byte_offset + 1;
    scanned_signature_row_count++;
  }

  return maximum_popup_horizontal_scroll_column_count;
}

static uint32_t
pick_signature_row_background(int selected) {
  if (selected)
    return area512_theme_selected_color();

  return area512_theme_box_color();
}

static uint32_t
pick_signature_emphasis_foreground(int selected) {
  if (selected)
    return area512_theme_background_color();

  return area512_theme_emphasis_color();
}

static void
paint_signature_row_background(Vim *vim, int selected) {
  VimCanvas *canvas = vim->active_canvas;

  canvas->clear_row(canvas->context);

  if (canvas->fill_row_span)
    canvas->fill_row_span(
      canvas->context,
      0,
      vim->screen.width,
      pick_signature_row_background(selected)
    );
}

static int
draw_signature_part(
  Vim *vim,
  int column,
  const char *signature,
  int start_byte_offset,
  int end_byte_offset,
  int name_byte_length,
  int selected
) {

  VimCanvas *canvas = vim->active_canvas;

  // name part
  if (
    start_byte_offset < name_byte_length &&
    start_byte_offset < end_byte_offset
  ) {

    int name_end_byte_offset =
      minimum_signature_value(end_byte_offset, name_byte_length);

    canvas->draw_row_text(
      canvas->context,
      column,
      signature + start_byte_offset,
      name_end_byte_offset - start_byte_offset,
      pick_signature_emphasis_foreground(selected),
      pick_signature_row_background(selected),
      0
    );

    column += vim_display_width(
      signature + start_byte_offset,
      name_end_byte_offset - start_byte_offset
    );

    start_byte_offset = name_end_byte_offset;
  }

  // remaining signature part
  if (start_byte_offset < end_byte_offset) {
    canvas->draw_row_text(
      canvas->context,
      column,
      signature + start_byte_offset,
      end_byte_offset - start_byte_offset,
      selected ? area512_theme_background_color() : area512_theme_text_color(),
      pick_signature_row_background(selected),
      0
    );

    column += vim_display_width(
      signature + start_byte_offset,
      end_byte_offset - start_byte_offset
    );
  }

  return column;
}

int
draw_signature_rows(
  Vim *vim,
  int screen_row,
  int rows_available,
  const char *signature,
  int signature_byte_length,
  int name_byte_length,
  const char *class_name,
  int selected,
  int popup_horizontal_scroll_column_count
) {

  VimCanvas *canvas = vim->active_canvas;

  if (name_byte_length > signature_byte_length)
    name_byte_length = signature_byte_length;

  int available_signature_column_count = signature_text_width(vim);
  int signature_row_start_byte_offset = 0;
  int drawn_signature_row_count = 0;

  while (
    signature_row_start_byte_offset <= signature_byte_length &&
    drawn_signature_row_count < rows_available
  ) {

    int signature_row_end_byte_offset = signature_row_start_byte_offset;

    while (
      signature_row_end_byte_offset < signature_byte_length &&
      signature[signature_row_end_byte_offset] != '\n'
    ) {

      signature_row_end_byte_offset++;
    }

    int signature_row_byte_length =
      signature_row_end_byte_offset - signature_row_start_byte_offset;

    int hidden_signature_row_byte_length =
      vim_column_to_byte(
        signature + signature_row_start_byte_offset,
        signature_row_byte_length,
        popup_horizontal_scroll_column_count
      );

    int visible_signature_row_byte_length =
      vim_column_to_byte(
        signature + signature_row_start_byte_offset +
          hidden_signature_row_byte_length,
        signature_row_byte_length - hidden_signature_row_byte_length,
        available_signature_column_count
      );

    paint_signature_row_background(vim, selected);

    draw_signature_part(
      vim,
      SIGNATURE_TEXT_START_COLUMN,
      signature + signature_row_start_byte_offset,
      hidden_signature_row_byte_length,
      hidden_signature_row_byte_length + visible_signature_row_byte_length,
      name_byte_length,
      selected
    );

    if (drawn_signature_row_count == 0) {
      int signature_row_column_count =
        vim_display_width(
          signature + signature_row_start_byte_offset,
          signature_row_byte_length
        );

      int class_name_width = signature_class_name_width(class_name);

      if (
        can_draw_signature_class_name(
          vim,
          signature_row_column_count,
          class_name_width
        )
      ) {
        canvas->draw_row_text(
          canvas->context,
          vim->screen.width - 1 - class_name_width,
          class_name,
          (int)strlen(class_name),
          pick_signature_emphasis_foreground(selected),
          pick_signature_row_background(selected),
          0
        );
      }
    }

    canvas->push_row(
      canvas->context,
      screen_row + drawn_signature_row_count
    );

    drawn_signature_row_count++;

    if (signature_row_end_byte_offset == signature_byte_length)
      break;

    signature_row_start_byte_offset = signature_row_end_byte_offset + 1;
  }

  return drawn_signature_row_count;
}
