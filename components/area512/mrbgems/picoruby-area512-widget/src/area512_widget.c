#include <stdbool.h>
#include <stdint.h>

#if defined(PICORB_VM_MRUBYC)

#include "area512_hal.h"
#include "core/widget.h"
#include "io-console.h"

#include <mrubyc.h>
#include <stdlib.h>
#include <string.h>

// -----------------------------------------------------------------------------
// Argument and value helpers
// -----------------------------------------------------------------------------

static int
ensure_argument_count_or_raise(
  mrbc_vm *virtual_machine,
  int argument_count,
  int minimum
) {

  if (argument_count < minimum) {
    mrbc_raise(
      virtual_machine,
      MRBC_CLASS(ArgumentError),
      "wrong number of arguments"
    );
    return 0;
  }

  return 1;
}

static int
fetch_integer_or_raise(mrbc_vm *virtual_machine, mrbc_value *v, int index) {

  if (v[index].tt != MRBC_TT_INTEGER) {
    mrbc_raise(virtual_machine, MRBC_CLASS(TypeError), "expected Integer");
    return 0;
  }

  return (int)v[index].i;
}

static uint32_t
fetch_color_or_raise(mrbc_vm *virtual_machine, mrbc_value *v, int index) {

  return (uint32_t)fetch_integer_or_raise(virtual_machine, v, index);
}

static const char *
fetch_string_or_raise(mrbc_vm *virtual_machine, mrbc_value *v, int index) {

  if (v[index].tt != MRBC_TT_STRING) {
    mrbc_raise(virtual_machine, MRBC_CLASS(TypeError), "expected String");
    return "";
  }

  return (const char *)v[index].string->data;
}

static mrbc_value *
fetch_array_or_raise(mrbc_vm *virtual_machine, mrbc_value *v, int index) {

  if (v[index].tt != MRBC_TT_ARRAY) {
    mrbc_raise(virtual_machine, MRBC_CLASS(TypeError), "expected Array");
    return NULL;
  }

  return &v[index];
}

static void *
fetch_sprite_handle_or_raise(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int index
) {

  if (v[index].tt != MRBC_TT_OBJECT) {
    mrbc_raise(virtual_machine, MRBC_CLASS(TypeError), "expected Sprite");
    return NULL;
  }

  return *(void **)v[index].instance->data;
}

static int
value_is_truthy(const mrbc_value *value) {
  return value->tt != MRBC_TT_NIL && value->tt != MRBC_TT_FALSE;
}

static void
copy_value_string(
  char *destination,
  int destination_size,
  const mrbc_value *value
) {

  int length = value->string->size;

  if (length >= destination_size)
    length = destination_size - 1;

  memcpy(destination, value->string->data, length);
  destination[length] = 0;
}

static int
array_strings(
  mrbc_vm *virtual_machine,
  mrbc_value *array,
  const char **strings,
  int maximum
) {

  int count = mrbc_array_size(array);

  if (count > maximum)
    count = maximum;

  for (int i = 0; i < count; i++) {
    mrbc_value item = mrbc_array_get(array, i);

    if (item.tt != MRBC_TT_STRING) {
      mrbc_raise(virtual_machine, MRBC_CLASS(TypeError), "expected String");
      return -1;
    }

    strings[i] = (const char *)item.string->data;
  }

  return count;
}

static int
array_integers(
  mrbc_vm *virtual_machine,
  mrbc_value *array,
  int *integers,
  int maximum
) {

  int count = mrbc_array_size(array);

  if (count > maximum)
    count = maximum;

  for (int i = 0; i < count; i++) {
    mrbc_value item = mrbc_array_get(array, i);

    if (item.tt != MRBC_TT_INTEGER) {
      mrbc_raise(virtual_machine, MRBC_CLASS(TypeError), "expected Integer");
      return -1;
    }

    integers[i] = (int)item.i;
  }

  return count;
}

// -----------------------------------------------------------------------------
// Theme and metrics
// -----------------------------------------------------------------------------

#define DEFINE_INTEGER_METHOD(function_name, value_expression)                 \
  static void function_name(                                                   \
    mrbc_vm *virtual_machine,                                                  \
    mrbc_value *v,                                                             \
    int argument_count                                                         \
  ) {                                                                          \
    (void)virtual_machine;                                                     \
    (void)v;                                                                   \
    (void)argument_count;                                                      \
    SET_INT_RETURN(value_expression);                                          \
  }

DEFINE_INTEGER_METHOD(c_widget_bg, WIDGET_COLOR_BG)
DEFINE_INTEGER_METHOD(c_widget_amber, WIDGET_COLOR_AMBER)
DEFINE_INTEGER_METHOD(c_widget_dim, WIDGET_COLOR_DIM)
DEFINE_INTEGER_METHOD(c_widget_gold, WIDGET_COLOR_GOLD)
DEFINE_INTEGER_METHOD(c_widget_dark, WIDGET_COLOR_DARK)
DEFINE_INTEGER_METHOD(c_widget_char_width, WIDGET_CHAR_WIDTH)
DEFINE_INTEGER_METHOD(c_widget_row_height, WIDGET_ROW_HEIGHT)
DEFINE_INTEGER_METHOD(c_widget_header_height, WIDGET_HEADER_HEIGHT)
DEFINE_INTEGER_METHOD(c_widget_body_top, area512_widget_body_top())
DEFINE_INTEGER_METHOD(c_widget_body_bottom, area512_widget_body_bottom())
DEFINE_INTEGER_METHOD(c_widget_body_height, area512_widget_body_height())

static void
c_widget_text_width(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int argument_count
) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 2))
    return;

  void *sprite = fetch_sprite_handle_or_raise(virtual_machine, v, 1);
  const char *text = fetch_string_or_raise(virtual_machine, v, 2);

  SET_INT_RETURN(area512_widget_text_width(sprite, text));
}

static void
c_widget_clip(mrbc_vm *virtual_machine, mrbc_value *v, int argument_count) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 3))
    return;

  void *sprite = fetch_sprite_handle_or_raise(virtual_machine, v, 1);
  const char *text = fetch_string_or_raise(virtual_machine, v, 2);
  int width = fetch_integer_or_raise(virtual_machine, v, 3);
  char clipped[WIDGET_TEXT_VIEW_MAX];

  area512_widget_clip(sprite, text, width, clipped, sizeof clipped);

  mrbc_value result = mrbc_string_new_cstr(virtual_machine, clipped);
  SET_RETURN(result);
}

static void
c_widget_read_key(mrbc_vm *virtual_machine, mrbc_value *v, int argument_count) {

  (void)v;
  (void)argument_count;

  char byte_string[2];
  const char *name =
    area512_widget_key_name(area512_widget_read_key(), byte_string);
  mrbc_value result = mrbc_string_new_cstr(virtual_machine, name);

  SET_RETURN(result);
}

// -----------------------------------------------------------------------------
// Bars, panels, and text
// -----------------------------------------------------------------------------

static void
c_widget_header(mrbc_vm *virtual_machine, mrbc_value *v, int argument_count) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 2))
    return;

  void *sprite = fetch_sprite_handle_or_raise(virtual_machine, v, 1);
  const char *title = fetch_string_or_raise(virtual_machine, v, 2);
  const char *right_text =
    argument_count >= 3 ? fetch_string_or_raise(virtual_machine, v, 3) : "";

  area512_widget_draw_header(sprite, title, right_text);
  SET_NIL_RETURN();
}

static void
c_widget_footer(mrbc_vm *virtual_machine, mrbc_value *v, int argument_count) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 2))
    return;

  area512_widget_draw_footer(
    fetch_sprite_handle_or_raise(virtual_machine, v, 1),
    fetch_string_or_raise(virtual_machine, v, 2)
  );
  SET_NIL_RETURN();
}

static void
c_widget_hints(mrbc_vm *virtual_machine, mrbc_value *v, int argument_count) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 2))
    return;

  void *sprite = fetch_sprite_handle_or_raise(virtual_machine, v, 1);
  mrbc_value *array = fetch_array_or_raise(virtual_machine, v, 2);

  if (!array)
    return;

  WidgetHint hints[WIDGET_MENU_MAX_ITEMS];
  int hint_count = mrbc_array_size(array);

  if (hint_count > WIDGET_MENU_MAX_ITEMS)
    hint_count = WIDGET_MENU_MAX_ITEMS;

  for (int i = 0; i < hint_count; i++) {
    mrbc_value pair = mrbc_array_get(array, i);

    if (pair.tt != MRBC_TT_ARRAY || mrbc_array_size(&pair) < 2) {
      mrbc_raise(virtual_machine, MRBC_CLASS(TypeError), "expected Array pair");
      return;
    }

    mrbc_value key = mrbc_array_get(&pair, 0);
    mrbc_value label = mrbc_array_get(&pair, 1);

    if (key.tt != MRBC_TT_STRING || label.tt != MRBC_TT_STRING) {
      mrbc_raise(virtual_machine, MRBC_CLASS(TypeError), "expected String");
      return;
    }

    hints[i].key = (const char *)key.string->data;
    hints[i].label = (const char *)label.string->data;
  }

  area512_widget_draw_hints(sprite, hints, hint_count);
  SET_NIL_RETURN();
}

static void
c_widget_separator(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int argument_count
) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 2))
    return;

  area512_widget_draw_separator(
    fetch_sprite_handle_or_raise(virtual_machine, v, 1),
    fetch_integer_or_raise(virtual_machine, v, 2)
  );
  SET_NIL_RETURN();
}

static void
c_widget_vertical_separator(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int argument_count
) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 4))
    return;

  area512_widget_draw_vertical_separator(
    fetch_sprite_handle_or_raise(virtual_machine, v, 1),
    fetch_integer_or_raise(virtual_machine, v, 2),
    fetch_integer_or_raise(virtual_machine, v, 3),
    fetch_integer_or_raise(virtual_machine, v, 4)
  );
  SET_NIL_RETURN();
}

static void
c_widget_tabs(mrbc_vm *virtual_machine, mrbc_value *v, int argument_count) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 4))
    return;

  void *sprite = fetch_sprite_handle_or_raise(virtual_machine, v, 1);
  int y = fetch_integer_or_raise(virtual_machine, v, 2);
  mrbc_value *array = fetch_array_or_raise(virtual_machine, v, 3);

  if (!array)
    return;

  const char *labels[WIDGET_MENU_MAX_ITEMS];
  int label_count =
    array_strings(virtual_machine, array, labels, WIDGET_MENU_MAX_ITEMS);

  if (label_count < 0)
    return;

  area512_widget_draw_tabs(
    sprite,
    y,
    labels,
    label_count,
    fetch_integer_or_raise(virtual_machine, v, 4)
  );
  SET_NIL_RETURN();
}

static void
c_widget_battery(mrbc_vm *virtual_machine, mrbc_value *v, int argument_count) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 3))
    return;

  area512_widget_draw_battery(
    fetch_sprite_handle_or_raise(virtual_machine, v, 1),
    fetch_integer_or_raise(virtual_machine, v, 2),
    fetch_integer_or_raise(virtual_machine, v, 3)
  );
  SET_NIL_RETURN();
}

static void
c_widget_splash(mrbc_vm *virtual_machine, mrbc_value *v, int argument_count) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 2))
    return;

  const char *subtitle =
    argument_count >= 3 ? fetch_string_or_raise(virtual_machine, v, 3) : "";

  area512_widget_draw_splash(
    fetch_sprite_handle_or_raise(virtual_machine, v, 1),
    fetch_string_or_raise(virtual_machine, v, 2),
    subtitle
  );
  SET_NIL_RETURN();
}

static void
c_widget_panel(mrbc_vm *virtual_machine, mrbc_value *v, int argument_count) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 5))
    return;

  area512_widget_draw_panel(
    fetch_sprite_handle_or_raise(virtual_machine, v, 1),
    fetch_integer_or_raise(virtual_machine, v, 2),
    fetch_integer_or_raise(virtual_machine, v, 3),
    fetch_integer_or_raise(virtual_machine, v, 4),
    fetch_integer_or_raise(virtual_machine, v, 5)
  );
  SET_NIL_RETURN();
}

static void
c_widget_titled_panel(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int argument_count
) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 6))
    return;

  area512_widget_draw_titled_panel(
    fetch_sprite_handle_or_raise(virtual_machine, v, 1),
    fetch_integer_or_raise(virtual_machine, v, 2),
    fetch_integer_or_raise(virtual_machine, v, 3),
    fetch_integer_or_raise(virtual_machine, v, 4),
    fetch_integer_or_raise(virtual_machine, v, 5),
    fetch_string_or_raise(virtual_machine, v, 6)
  );
  SET_NIL_RETURN();
}

static void
c_widget_toast(mrbc_vm *virtual_machine, mrbc_value *v, int argument_count) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 2))
    return;

  area512_widget_draw_toast(
    fetch_sprite_handle_or_raise(virtual_machine, v, 1),
    fetch_string_or_raise(virtual_machine, v, 2)
  );
  SET_NIL_RETURN();
}

static void
c_widget_text_center(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int argument_count
) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 3))
    return;

  uint32_t color = argument_count >= 4
                     ? fetch_color_or_raise(virtual_machine, v, 4)
                     : WIDGET_COLOR_DIM;

  area512_widget_draw_text_center(
    fetch_sprite_handle_or_raise(virtual_machine, v, 1),
    fetch_integer_or_raise(virtual_machine, v, 2),
    fetch_string_or_raise(virtual_machine, v, 3),
    color
  );
  SET_NIL_RETURN();
}

static void
c_widget_text_right(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int argument_count
) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 4))
    return;

  uint32_t color = argument_count >= 5
                     ? fetch_color_or_raise(virtual_machine, v, 5)
                     : WIDGET_COLOR_DIM;

  area512_widget_draw_text_right(
    fetch_sprite_handle_or_raise(virtual_machine, v, 1),
    fetch_integer_or_raise(virtual_machine, v, 2),
    fetch_integer_or_raise(virtual_machine, v, 3),
    fetch_string_or_raise(virtual_machine, v, 4),
    color
  );
  SET_NIL_RETURN();
}

static void
c_widget_center_lines(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int argument_count
) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 2))
    return;

  void *sprite = fetch_sprite_handle_or_raise(virtual_machine, v, 1);
  mrbc_value *array = fetch_array_or_raise(virtual_machine, v, 2);

  if (!array)
    return;

  WidgetColoredLine lines[WIDGET_MENU_MAX_ITEMS];
  int line_count = mrbc_array_size(array);

  if (line_count > WIDGET_MENU_MAX_ITEMS)
    line_count = WIDGET_MENU_MAX_ITEMS;

  for (int i = 0; i < line_count; i++) {
    mrbc_value row = mrbc_array_get(array, i);

    lines[i].color = WIDGET_COLOR_DIM;

    if (row.tt == MRBC_TT_STRING) {
      lines[i].text = (const char *)row.string->data;
      continue;
    }

    if (row.tt != MRBC_TT_ARRAY || mrbc_array_size(&row) == 0) {
      mrbc_raise(
        virtual_machine,
        MRBC_CLASS(TypeError),
        "expected String or Array"
      );
      return;
    }

    mrbc_value text = mrbc_array_get(&row, 0);

    if (text.tt != MRBC_TT_STRING) {
      mrbc_raise(virtual_machine, MRBC_CLASS(TypeError), "expected String");
      return;
    }

    lines[i].text = (const char *)text.string->data;

    if (mrbc_array_size(&row) >= 2) {
      mrbc_value color = mrbc_array_get(&row, 1);

      if (color.tt != MRBC_TT_INTEGER) {
        mrbc_raise(virtual_machine, MRBC_CLASS(TypeError), "expected Integer");
        return;
      }

      lines[i].color = (uint32_t)color.i;
    }
  }

  area512_widget_draw_center_lines(sprite, lines, line_count);
  SET_NIL_RETURN();
}

static void
c_widget_wrap_text(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int argument_count
) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 6))
    return;

  uint32_t color = argument_count >= 7
                     ? fetch_color_or_raise(virtual_machine, v, 7)
                     : WIDGET_COLOR_DIM;

  int line_count = area512_widget_draw_wrap_text(
    fetch_sprite_handle_or_raise(virtual_machine, v, 1),
    fetch_integer_or_raise(virtual_machine, v, 2),
    fetch_integer_or_raise(virtual_machine, v, 3),
    fetch_integer_or_raise(virtual_machine, v, 4),
    fetch_integer_or_raise(virtual_machine, v, 5),
    fetch_string_or_raise(virtual_machine, v, 6),
    color
  );

  SET_INT_RETURN(line_count);
}

static void
c_widget_marquee(mrbc_vm *virtual_machine, mrbc_value *v, int argument_count) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 6))
    return;

  int width = area512_widget_draw_marquee(
    fetch_sprite_handle_or_raise(virtual_machine, v, 1),
    fetch_integer_or_raise(virtual_machine, v, 2),
    fetch_integer_or_raise(virtual_machine, v, 3),
    fetch_integer_or_raise(virtual_machine, v, 4),
    fetch_string_or_raise(virtual_machine, v, 5),
    fetch_integer_or_raise(virtual_machine, v, 6)
  );

  SET_INT_RETURN(width);
}

static void
c_widget_big_text(mrbc_vm *virtual_machine, mrbc_value *v, int argument_count) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 4))
    return;

  uint32_t color = argument_count >= 5
                     ? fetch_color_or_raise(virtual_machine, v, 5)
                     : WIDGET_COLOR_GOLD;

  area512_widget_draw_big_text(
    fetch_sprite_handle_or_raise(virtual_machine, v, 1),
    fetch_integer_or_raise(virtual_machine, v, 2),
    fetch_integer_or_raise(virtual_machine, v, 3),
    fetch_string_or_raise(virtual_machine, v, 4),
    color
  );
  SET_NIL_RETURN();
}

// -----------------------------------------------------------------------------
// Tables, indicators, charts, and controls
// -----------------------------------------------------------------------------

static void
c_widget_cell(mrbc_vm *virtual_machine, mrbc_value *v, int argument_count) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 7))
    return;

  area512_widget_draw_cell(
    fetch_sprite_handle_or_raise(virtual_machine, v, 1),
    fetch_integer_or_raise(virtual_machine, v, 2),
    fetch_integer_or_raise(virtual_machine, v, 3),
    fetch_integer_or_raise(virtual_machine, v, 4),
    fetch_integer_or_raise(virtual_machine, v, 5),
    fetch_string_or_raise(virtual_machine, v, 6),
    value_is_truthy(&v[7])
  );
  SET_NIL_RETURN();
}

static int
table_arguments(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int widths[WIDGET_TABLE_MAX_COLUMNS],
  const char *texts[WIDGET_TABLE_MAX_COLUMNS]
) {

  mrbc_value *width_array = fetch_array_or_raise(virtual_machine, v, 4);
  mrbc_value *text_array = fetch_array_or_raise(virtual_machine, v, 5);

  if (!width_array || !text_array)
    return -1;

  int width_count = array_integers(
    virtual_machine,
    width_array,
    widths,
    WIDGET_TABLE_MAX_COLUMNS
  );
  int text_count =
    array_strings(virtual_machine, text_array, texts, WIDGET_TABLE_MAX_COLUMNS);

  if (width_count < 0 || text_count < 0)
    return -1;

  return width_count < text_count ? width_count : text_count;
}

static void
c_widget_table_header(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int argument_count
) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 5))
    return;

  int widths[WIDGET_TABLE_MAX_COLUMNS];
  const char *labels[WIDGET_TABLE_MAX_COLUMNS];
  int column_count = table_arguments(virtual_machine, v, widths, labels);

  if (column_count < 0)
    return;

  area512_widget_draw_table_header(
    fetch_sprite_handle_or_raise(virtual_machine, v, 1),
    fetch_integer_or_raise(virtual_machine, v, 2),
    fetch_integer_or_raise(virtual_machine, v, 3),
    widths,
    labels,
    column_count
  );
  SET_NIL_RETURN();
}

static void
c_widget_table_row(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int argument_count
) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 6))
    return;

  int widths[WIDGET_TABLE_MAX_COLUMNS];
  const char *texts[WIDGET_TABLE_MAX_COLUMNS];
  int column_count = table_arguments(virtual_machine, v, widths, texts);

  if (column_count < 0)
    return;

  area512_widget_draw_table_row(
    fetch_sprite_handle_or_raise(virtual_machine, v, 1),
    fetch_integer_or_raise(virtual_machine, v, 2),
    fetch_integer_or_raise(virtual_machine, v, 3),
    widths,
    texts,
    column_count,
    value_is_truthy(&v[6])
  );
  SET_NIL_RETURN();
}

static void
c_widget_field(mrbc_vm *virtual_machine, mrbc_value *v, int argument_count) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 7))
    return;

  area512_widget_draw_field(
    fetch_sprite_handle_or_raise(virtual_machine, v, 1),
    fetch_integer_or_raise(virtual_machine, v, 2),
    fetch_integer_or_raise(virtual_machine, v, 3),
    fetch_integer_or_raise(virtual_machine, v, 4),
    fetch_string_or_raise(virtual_machine, v, 5),
    fetch_string_or_raise(virtual_machine, v, 6),
    value_is_truthy(&v[7])
  );
  SET_NIL_RETURN();
}

static void
c_widget_gauge(mrbc_vm *virtual_machine, mrbc_value *v, int argument_count) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 6))
    return;

  area512_widget_draw_gauge(
    fetch_sprite_handle_or_raise(virtual_machine, v, 1),
    fetch_integer_or_raise(virtual_machine, v, 2),
    fetch_integer_or_raise(virtual_machine, v, 3),
    fetch_integer_or_raise(virtual_machine, v, 4),
    fetch_integer_or_raise(virtual_machine, v, 5),
    fetch_integer_or_raise(virtual_machine, v, 6)
  );
  SET_NIL_RETURN();
}

static void
c_widget_slider(mrbc_vm *virtual_machine, mrbc_value *v, int argument_count) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 7))
    return;

  area512_widget_draw_slider(
    fetch_sprite_handle_or_raise(virtual_machine, v, 1),
    fetch_integer_or_raise(virtual_machine, v, 2),
    fetch_integer_or_raise(virtual_machine, v, 3),
    fetch_integer_or_raise(virtual_machine, v, 4),
    fetch_integer_or_raise(virtual_machine, v, 5),
    fetch_integer_or_raise(virtual_machine, v, 6),
    value_is_truthy(&v[7])
  );
  SET_NIL_RETURN();
}

static void
c_widget_scrollbar(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int argument_count
) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 7))
    return;

  area512_widget_draw_scrollbar(
    fetch_sprite_handle_or_raise(virtual_machine, v, 1),
    fetch_integer_or_raise(virtual_machine, v, 2),
    fetch_integer_or_raise(virtual_machine, v, 3),
    fetch_integer_or_raise(virtual_machine, v, 4),
    fetch_integer_or_raise(virtual_machine, v, 5),
    fetch_integer_or_raise(virtual_machine, v, 6),
    fetch_integer_or_raise(virtual_machine, v, 7)
  );
  SET_NIL_RETURN();
}

static void
c_widget_horizontal_scrollbar(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int argument_count
) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 7))
    return;

  area512_widget_draw_horizontal_scrollbar(
    fetch_sprite_handle_or_raise(virtual_machine, v, 1),
    fetch_integer_or_raise(virtual_machine, v, 2),
    fetch_integer_or_raise(virtual_machine, v, 3),
    fetch_integer_or_raise(virtual_machine, v, 4),
    fetch_integer_or_raise(virtual_machine, v, 5),
    fetch_integer_or_raise(virtual_machine, v, 6),
    fetch_integer_or_raise(virtual_machine, v, 7)
  );
  SET_NIL_RETURN();
}

static void
c_widget_badge(mrbc_vm *virtual_machine, mrbc_value *v, int argument_count) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 4))
    return;

  area512_widget_draw_badge(
    fetch_sprite_handle_or_raise(virtual_machine, v, 1),
    fetch_integer_or_raise(virtual_machine, v, 2),
    fetch_integer_or_raise(virtual_machine, v, 3),
    fetch_string_or_raise(virtual_machine, v, 4)
  );
  SET_NIL_RETURN();
}

static void
c_widget_busy(mrbc_vm *virtual_machine, mrbc_value *v, int argument_count) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 4))
    return;

  area512_widget_draw_busy(
    fetch_sprite_handle_or_raise(virtual_machine, v, 1),
    fetch_integer_or_raise(virtual_machine, v, 2),
    fetch_integer_or_raise(virtual_machine, v, 3),
    fetch_integer_or_raise(virtual_machine, v, 4)
  );
  SET_NIL_RETURN();
}

static void
c_widget_page_dots(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int argument_count
) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 4))
    return;

  area512_widget_draw_page_dots(
    fetch_sprite_handle_or_raise(virtual_machine, v, 1),
    fetch_integer_or_raise(virtual_machine, v, 2),
    fetch_integer_or_raise(virtual_machine, v, 3),
    fetch_integer_or_raise(virtual_machine, v, 4)
  );
  SET_NIL_RETURN();
}

static int
chart_arguments(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int chart_values[WIDGET_CHART_MAX_VALUES]
) {

  mrbc_value *array = fetch_array_or_raise(virtual_machine, v, 6);

  if (!array)
    return -1;

  return array_integers(
    virtual_machine,
    array,
    chart_values,
    WIDGET_CHART_MAX_VALUES
  );
}

static void
c_widget_bar_chart(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int argument_count
) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 7))
    return;

  int chart_values[WIDGET_CHART_MAX_VALUES];
  int value_count = chart_arguments(virtual_machine, v, chart_values);

  if (value_count < 0)
    return;

  area512_widget_draw_bar_chart(
    fetch_sprite_handle_or_raise(virtual_machine, v, 1),
    fetch_integer_or_raise(virtual_machine, v, 2),
    fetch_integer_or_raise(virtual_machine, v, 3),
    fetch_integer_or_raise(virtual_machine, v, 4),
    fetch_integer_or_raise(virtual_machine, v, 5),
    chart_values,
    value_count,
    fetch_integer_or_raise(virtual_machine, v, 7)
  );
  SET_NIL_RETURN();
}

static void
c_widget_line_chart(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int argument_count
) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 7))
    return;

  int chart_values[WIDGET_CHART_MAX_VALUES];
  int value_count = chart_arguments(virtual_machine, v, chart_values);

  if (value_count < 0)
    return;

  area512_widget_draw_line_chart(
    fetch_sprite_handle_or_raise(virtual_machine, v, 1),
    fetch_integer_or_raise(virtual_machine, v, 2),
    fetch_integer_or_raise(virtual_machine, v, 3),
    fetch_integer_or_raise(virtual_machine, v, 4),
    fetch_integer_or_raise(virtual_machine, v, 5),
    chart_values,
    value_count,
    fetch_integer_or_raise(virtual_machine, v, 7)
  );
  SET_NIL_RETURN();
}

static void
c_widget_button(mrbc_vm *virtual_machine, mrbc_value *v, int argument_count) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 6))
    return;

  area512_widget_draw_button(
    fetch_sprite_handle_or_raise(virtual_machine, v, 1),
    fetch_integer_or_raise(virtual_machine, v, 2),
    fetch_integer_or_raise(virtual_machine, v, 3),
    fetch_integer_or_raise(virtual_machine, v, 4),
    fetch_string_or_raise(virtual_machine, v, 5),
    value_is_truthy(&v[6])
  );
  SET_NIL_RETURN();
}

static void
c_widget_checkbox(mrbc_vm *virtual_machine, mrbc_value *v, int argument_count) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 6))
    return;

  area512_widget_draw_checkbox(
    fetch_sprite_handle_or_raise(virtual_machine, v, 1),
    fetch_integer_or_raise(virtual_machine, v, 2),
    fetch_integer_or_raise(virtual_machine, v, 3),
    fetch_string_or_raise(virtual_machine, v, 4),
    value_is_truthy(&v[5]),
    value_is_truthy(&v[6])
  );
  SET_NIL_RETURN();
}

static void
c_widget_radio(mrbc_vm *virtual_machine, mrbc_value *v, int argument_count) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 6))
    return;

  area512_widget_draw_radio(
    fetch_sprite_handle_or_raise(virtual_machine, v, 1),
    fetch_integer_or_raise(virtual_machine, v, 2),
    fetch_integer_or_raise(virtual_machine, v, 3),
    fetch_string_or_raise(virtual_machine, v, 4),
    value_is_truthy(&v[5]),
    value_is_truthy(&v[6])
  );
  SET_NIL_RETURN();
}

static void
c_widget_toggle(mrbc_vm *virtual_machine, mrbc_value *v, int argument_count) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 5))
    return;

  area512_widget_draw_toggle(
    fetch_sprite_handle_or_raise(virtual_machine, v, 1),
    fetch_integer_or_raise(virtual_machine, v, 2),
    fetch_integer_or_raise(virtual_machine, v, 3),
    value_is_truthy(&v[4]),
    value_is_truthy(&v[5])
  );
  SET_NIL_RETURN();
}

static void
c_widget_spinner(mrbc_vm *virtual_machine, mrbc_value *v, int argument_count) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 6))
    return;

  area512_widget_draw_spinner(
    fetch_sprite_handle_or_raise(virtual_machine, v, 1),
    fetch_integer_or_raise(virtual_machine, v, 2),
    fetch_integer_or_raise(virtual_machine, v, 3),
    fetch_integer_or_raise(virtual_machine, v, 4),
    fetch_string_or_raise(virtual_machine, v, 5),
    value_is_truthy(&v[6])
  );
  SET_NIL_RETURN();
}

// -----------------------------------------------------------------------------
// Modals
// -----------------------------------------------------------------------------

static void
c_widget_input(mrbc_vm *virtual_machine, mrbc_value *v, int argument_count) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 2))
    return;

  void *sprite = fetch_sprite_handle_or_raise(virtual_machine, v, 1);
  const char *label = fetch_string_or_raise(virtual_machine, v, 2);
  const char *initial =
    argument_count >= 3 ? fetch_string_or_raise(virtual_machine, v, 3) : "";
  char output[WIDGET_INPUT_MAX];

  io_raw_bang(false);
  int confirmed =
    area512_widget_run_input_modal(sprite, label, initial, output);
  io_cooked_bang();

  if (!confirmed) {
    SET_NIL_RETURN();
    return;
  }

  mrbc_value result = mrbc_string_new_cstr(virtual_machine, output);
  SET_RETURN(result);
}

static void
c_widget_input_number(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int argument_count
) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 5))
    return;

  void *sprite = fetch_sprite_handle_or_raise(virtual_machine, v, 1);
  const char *label = fetch_string_or_raise(virtual_machine, v, 2);
  int initial = fetch_integer_or_raise(virtual_machine, v, 3);
  int minimum = fetch_integer_or_raise(virtual_machine, v, 4);
  int maximum = fetch_integer_or_raise(virtual_machine, v, 5);
  int result;

  io_raw_bang(false);
  int confirmed = area512_widget_run_number_modal(
    sprite,
    label,
    initial,
    minimum,
    maximum,
    &result
  );
  io_cooked_bang();

  if (confirmed)
    SET_INT_RETURN(result);
  else
    SET_NIL_RETURN();
}

static void
c_widget_confirm(mrbc_vm *virtual_machine, mrbc_value *v, int argument_count) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 2))
    return;

  void *sprite = fetch_sprite_handle_or_raise(virtual_machine, v, 1);
  const char *question = fetch_string_or_raise(virtual_machine, v, 2);

  io_raw_bang(false);
  int result = area512_widget_run_confirm_modal(sprite, question);
  io_cooked_bang();
  SET_BOOL_RETURN(result);
}

static int
modal_items(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int index,
  const char *items[WIDGET_MENU_MAX_ITEMS]
) {

  mrbc_value *array = fetch_array_or_raise(virtual_machine, v, index);

  if (!array)
    return -1;

  return array_strings(virtual_machine, array, items, WIDGET_MENU_MAX_ITEMS);
}

static void
c_widget_dialog(mrbc_vm *virtual_machine, mrbc_value *v, int argument_count) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 3))
    return;

  const char *buttons[WIDGET_MENU_MAX_ITEMS];
  int button_count = modal_items(virtual_machine, v, 3, buttons);

  if (button_count < 0)
    return;

  void *sprite = fetch_sprite_handle_or_raise(virtual_machine, v, 1);
  const char *message = fetch_string_or_raise(virtual_machine, v, 2);

  io_raw_bang(false);
  int result =
    area512_widget_run_dialog_modal(sprite, message, buttons, button_count);
  io_cooked_bang();

  if (result < 0)
    SET_NIL_RETURN();
  else
    SET_INT_RETURN(result);
}

static void
c_widget_menu(mrbc_vm *virtual_machine, mrbc_value *v, int argument_count) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 3))
    return;

  const char *items[WIDGET_MENU_MAX_ITEMS];
  int item_count = modal_items(virtual_machine, v, 3, items);

  if (item_count < 0)
    return;

  void *sprite = fetch_sprite_handle_or_raise(virtual_machine, v, 1);
  const char *title = fetch_string_or_raise(virtual_machine, v, 2);
  int initial =
    argument_count >= 4 ? fetch_integer_or_raise(virtual_machine, v, 4) : 0;

  io_raw_bang(false);
  int result =
    area512_widget_run_menu_modal(sprite, title, items, item_count, initial);
  io_cooked_bang();

  if (result < 0)
    SET_NIL_RETURN();
  else
    SET_INT_RETURN(result);
}

static void
c_widget_alert(mrbc_vm *virtual_machine, mrbc_value *v, int argument_count) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 2))
    return;

  void *sprite = fetch_sprite_handle_or_raise(virtual_machine, v, 1);
  const char *message = fetch_string_or_raise(virtual_machine, v, 2);

  io_raw_bang(false);
  area512_widget_run_alert_modal(sprite, message);
  io_cooked_bang();
  SET_NIL_RETURN();
}

// -----------------------------------------------------------------------------
// WidgetList
// -----------------------------------------------------------------------------

static WidgetList *
get_widget_list(mrbc_value *self) {
  return *(WidgetList **)self->instance->data;
}

static void
free_widget_list(mrbc_value *self) {
  free(get_widget_list(self));
}

static void
c_widget_list_new(mrbc_vm *virtual_machine, mrbc_value *v, int argument_count) {

  (void)argument_count;

  WidgetList *list = (WidgetList *)malloc(sizeof(WidgetList));

  if (!list) {
    mrbc_raise(
      virtual_machine,
      MRBC_CLASS(RuntimeError),
      "failed to allocate WidgetList"
    );
    return;
  }

  area512_widget_list_init(list);
  mrbc_value instance =
    mrbc_instance_new(virtual_machine, v->cls, sizeof(WidgetList *));
  *(WidgetList **)instance.instance->data = list;
  SET_RETURN(instance);
}

static void
c_widget_list_area(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int argument_count
) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 4))
    return;

  WidgetList *list = get_widget_list(v);

  list->x = fetch_integer_or_raise(virtual_machine, v, 1);
  list->y = fetch_integer_or_raise(virtual_machine, v, 2);
  list->w = fetch_integer_or_raise(virtual_machine, v, 3);
  list->h = fetch_integer_or_raise(virtual_machine, v, 4);
  area512_widget_list_set_index(list, list->index);
  SET_NIL_RETURN();
}

static void
c_widget_list_clear(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int argument_count
) {

  (void)virtual_machine;
  (void)argument_count;
  area512_widget_list_clear(get_widget_list(v));
  SET_NIL_RETURN();
}

static void
c_widget_list_add(mrbc_vm *virtual_machine, mrbc_value *v, int argument_count) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 1))
    return;

  const char *text = fetch_string_or_raise(virtual_machine, v, 1);
  const char *tag =
    argument_count >= 2 ? fetch_string_or_raise(virtual_machine, v, 2) : "";

  area512_widget_list_add(get_widget_list(v), text, tag);
  SET_NIL_RETURN();
}

static void
c_widget_list_set_empty_text(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int argument_count
) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 1))
    return;

  if (v[1].tt != MRBC_TT_STRING) {
    mrbc_raise(virtual_machine, MRBC_CLASS(TypeError), "expected String");
    return;
  }

  copy_value_string(
    get_widget_list(v)->empty_text,
    WIDGET_LIST_TEXT_MAX,
    &v[1]
  );
  mrbc_incref(&v[1]);
  SET_RETURN(v[1]);
}

static void
c_widget_list_set_show_marks(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int argument_count
) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 1))
    return;

  get_widget_list(v)->show_marks = value_is_truthy(&v[1]);
  SET_RETURN(v[1]);
}

static void
c_widget_list_toggle_mark(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int argument_count
) {

  (void)virtual_machine;
  (void)argument_count;

  WidgetList *list = get_widget_list(v);

  if (list->count > 0)
    list->marks[list->index] = !list->marks[list->index];

  SET_NIL_RETURN();
}

static void
c_widget_list_mark(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int argument_count
) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 2))
    return;

  WidgetList *list = get_widget_list(v);
  int index = fetch_integer_or_raise(virtual_machine, v, 1);

  if (index >= 0 && index < list->count)
    list->marks[index] = value_is_truthy(&v[2]);

  SET_NIL_RETURN();
}

static void
c_widget_list_marked(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int argument_count
) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 1))
    return;

  WidgetList *list = get_widget_list(v);
  int index = fetch_integer_or_raise(virtual_machine, v, 1);
  int marked = index >= 0 && index < list->count && list->marks[index];

  SET_BOOL_RETURN(marked);
}

static void
c_widget_list_count(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int argument_count
) {

  (void)virtual_machine;
  (void)argument_count;
  SET_INT_RETURN(get_widget_list(v)->count);
}

static void
c_widget_list_index(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int argument_count
) {

  (void)virtual_machine;
  (void)argument_count;
  SET_INT_RETURN(get_widget_list(v)->index);
}

static void
c_widget_list_set_index(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int argument_count
) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 1))
    return;

  WidgetList *list = get_widget_list(v);

  area512_widget_list_set_index(
    list,
    fetch_integer_or_raise(virtual_machine, v, 1)
  );
  SET_INT_RETURN(list->index);
}

static void
c_widget_list_top(mrbc_vm *virtual_machine, mrbc_value *v, int argument_count) {

  (void)virtual_machine;
  (void)argument_count;
  SET_INT_RETURN(get_widget_list(v)->top);
}

static void
c_widget_list_handle(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int argument_count
) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 1))
    return;

  int handled = area512_widget_list_handle(
    get_widget_list(v),
    fetch_string_or_raise(virtual_machine, v, 1)
  );
  SET_BOOL_RETURN(handled);
}

static void
c_widget_list_draw(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int argument_count
) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 1))
    return;

  area512_widget_list_draw(
    fetch_sprite_handle_or_raise(virtual_machine, v, 1),
    get_widget_list(v)
  );
  SET_NIL_RETURN();
}

// -----------------------------------------------------------------------------
// WidgetTextView
// -----------------------------------------------------------------------------

static WidgetTextView *
get_widget_text_view(mrbc_value *self) {
  return *(WidgetTextView **)self->instance->data;
}

static void
free_widget_text_view(mrbc_value *self) {
  free(get_widget_text_view(self));
}

static void
c_widget_text_view_new(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int argument_count
) {

  (void)argument_count;

  WidgetTextView *view = (WidgetTextView *)malloc(sizeof(WidgetTextView));

  if (!view) {
    mrbc_raise(
      virtual_machine,
      MRBC_CLASS(RuntimeError),
      "failed to allocate WidgetTextView"
    );
    return;
  }

  area512_widget_text_view_init(view);
  mrbc_value instance =
    mrbc_instance_new(virtual_machine, v->cls, sizeof(WidgetTextView *));
  *(WidgetTextView **)instance.instance->data = view;
  SET_RETURN(instance);
}

static void
c_widget_text_view_area(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int argument_count
) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 4))
    return;

  WidgetTextView *view = get_widget_text_view(v);

  view->x = fetch_integer_or_raise(virtual_machine, v, 1);
  view->y = fetch_integer_or_raise(virtual_machine, v, 2);
  view->w = fetch_integer_or_raise(virtual_machine, v, 3);
  view->h = fetch_integer_or_raise(virtual_machine, v, 4);
  SET_NIL_RETURN();
}

static void
c_widget_text_view_set_text(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int argument_count
) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 1))
    return;

  area512_widget_text_view_set_text(
    get_widget_text_view(v),
    fetch_string_or_raise(virtual_machine, v, 1)
  );
  mrbc_incref(&v[1]);
  SET_RETURN(v[1]);
}

static void
c_widget_text_view_scroll(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int argument_count
) {

  (void)virtual_machine;
  (void)argument_count;
  SET_INT_RETURN(get_widget_text_view(v)->scroll);
}

static void
c_widget_text_view_set_scroll(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int argument_count
) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 1))
    return;

  // Width measurement requires a Sprite. Clamp the lower bound here; draw and
  // handle clamp the upper bound once a Sprite is available.
  int scroll = fetch_integer_or_raise(virtual_machine, v, 1);

  if (scroll < 0)
    scroll = 0;

  get_widget_text_view(v)->scroll = scroll;
  SET_INT_RETURN(scroll);
}

static void
c_widget_text_view_handle(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int argument_count
) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 1))
    return;

  // handle has no Sprite argument in the Ruby API. ASCII metrics provide a
  // deterministic approximation; draw performs exact wrapping for rendering.
  WidgetTextView *view = get_widget_text_view(v);
  const char *key = fetch_string_or_raise(virtual_machine, v, 1);
  int delta = 0;

  if (strcmp(key, "UP") == 0 || strcmp(key, "k") == 0 || strcmp(key, ";") == 0)
    delta = -1;

  if (strcmp(key, "DOWN") == 0 || strcmp(key, "j") == 0 ||
      strcmp(key, ".") == 0)
    delta = 1;

  if (delta == 0) {
    SET_FALSE_RETURN();
    return;
  }

  view->scroll += delta;

  if (view->scroll < 0)
    view->scroll = 0;

  SET_TRUE_RETURN();
}

static void
c_widget_text_view_draw(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int argument_count
) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 1))
    return;

  void *sprite = fetch_sprite_handle_or_raise(virtual_machine, v, 1);
  WidgetTextView *view = get_widget_text_view(v);

  area512_widget_text_view_set_scroll(sprite, view, view->scroll);
  area512_widget_text_view_draw(sprite, view);
  SET_NIL_RETURN();
}

// -----------------------------------------------------------------------------
// Class registration
// -----------------------------------------------------------------------------

static void
define_widget_methods(mrbc_vm *virtual_machine, mrbc_class *widget_class) {
  mrbc_define_method(virtual_machine, widget_class, "bg", c_widget_bg);
  mrbc_define_method(virtual_machine, widget_class, "amber", c_widget_amber);
  mrbc_define_method(virtual_machine, widget_class, "dim", c_widget_dim);
  mrbc_define_method(virtual_machine, widget_class, "gold", c_widget_gold);
  mrbc_define_method(virtual_machine, widget_class, "dark", c_widget_dark);
  mrbc_define_method(
    virtual_machine,
    widget_class,
    "char_width",
    c_widget_char_width
  );
  mrbc_define_method(
    virtual_machine,
    widget_class,
    "row_height",
    c_widget_row_height
  );
  mrbc_define_method(
    virtual_machine,
    widget_class,
    "header_height",
    c_widget_header_height
  );
  mrbc_define_method(
    virtual_machine,
    widget_class,
    "body_top",
    c_widget_body_top
  );
  mrbc_define_method(
    virtual_machine,
    widget_class,
    "body_bottom",
    c_widget_body_bottom
  );
  mrbc_define_method(
    virtual_machine,
    widget_class,
    "body_height",
    c_widget_body_height
  );
  mrbc_define_method(
    virtual_machine,
    widget_class,
    "text_width",
    c_widget_text_width
  );
  mrbc_define_method(virtual_machine, widget_class, "clip", c_widget_clip);
  mrbc_define_method(
    virtual_machine,
    widget_class,
    "read_key",
    c_widget_read_key
  );

  mrbc_define_method(virtual_machine, widget_class, "header", c_widget_header);
  mrbc_define_method(virtual_machine, widget_class, "footer", c_widget_footer);
  mrbc_define_method(virtual_machine, widget_class, "hints", c_widget_hints);
  mrbc_define_method(
    virtual_machine,
    widget_class,
    "separator",
    c_widget_separator
  );
  mrbc_define_method(
    virtual_machine,
    widget_class,
    "vseparator",
    c_widget_vertical_separator
  );
  mrbc_define_method(virtual_machine, widget_class, "tabs", c_widget_tabs);
  mrbc_define_method(
    virtual_machine,
    widget_class,
    "battery",
    c_widget_battery
  );
  mrbc_define_method(virtual_machine, widget_class, "splash", c_widget_splash);
  mrbc_define_method(virtual_machine, widget_class, "panel", c_widget_panel);
  mrbc_define_method(
    virtual_machine,
    widget_class,
    "titled_panel",
    c_widget_titled_panel
  );
  mrbc_define_method(virtual_machine, widget_class, "toast", c_widget_toast);

  mrbc_define_method(
    virtual_machine,
    widget_class,
    "text_center",
    c_widget_text_center
  );
  mrbc_define_method(
    virtual_machine,
    widget_class,
    "text_right",
    c_widget_text_right
  );
  mrbc_define_method(
    virtual_machine,
    widget_class,
    "center_lines",
    c_widget_center_lines
  );
  mrbc_define_method(
    virtual_machine,
    widget_class,
    "wrap_text",
    c_widget_wrap_text
  );
  mrbc_define_method(
    virtual_machine,
    widget_class,
    "marquee",
    c_widget_marquee
  );
  mrbc_define_method(
    virtual_machine,
    widget_class,
    "big_text",
    c_widget_big_text
  );

  mrbc_define_method(virtual_machine, widget_class, "cell", c_widget_cell);
  mrbc_define_method(
    virtual_machine,
    widget_class,
    "table_header",
    c_widget_table_header
  );
  mrbc_define_method(
    virtual_machine,
    widget_class,
    "table_row",
    c_widget_table_row
  );
  mrbc_define_method(virtual_machine, widget_class, "field", c_widget_field);
  mrbc_define_method(virtual_machine, widget_class, "gauge", c_widget_gauge);
  mrbc_define_method(virtual_machine, widget_class, "slider", c_widget_slider);
  mrbc_define_method(
    virtual_machine,
    widget_class,
    "scrollbar",
    c_widget_scrollbar
  );
  mrbc_define_method(
    virtual_machine,
    widget_class,
    "hscrollbar",
    c_widget_horizontal_scrollbar
  );
  mrbc_define_method(virtual_machine, widget_class, "badge", c_widget_badge);
  mrbc_define_method(virtual_machine, widget_class, "busy", c_widget_busy);
  mrbc_define_method(
    virtual_machine,
    widget_class,
    "page_dots",
    c_widget_page_dots
  );
  mrbc_define_method(
    virtual_machine,
    widget_class,
    "bar_chart",
    c_widget_bar_chart
  );
  mrbc_define_method(
    virtual_machine,
    widget_class,
    "line_chart",
    c_widget_line_chart
  );

  mrbc_define_method(virtual_machine, widget_class, "button", c_widget_button);
  mrbc_define_method(
    virtual_machine,
    widget_class,
    "checkbox",
    c_widget_checkbox
  );
  mrbc_define_method(virtual_machine, widget_class, "radio", c_widget_radio);
  mrbc_define_method(virtual_machine, widget_class, "toggle", c_widget_toggle);
  mrbc_define_method(
    virtual_machine,
    widget_class,
    "spinner",
    c_widget_spinner
  );

  mrbc_define_method(virtual_machine, widget_class, "input", c_widget_input);
  mrbc_define_method(
    virtual_machine,
    widget_class,
    "input_number",
    c_widget_input_number
  );
  mrbc_define_method(
    virtual_machine,
    widget_class,
    "confirm",
    c_widget_confirm
  );
  mrbc_define_method(virtual_machine, widget_class, "dialog", c_widget_dialog);
  mrbc_define_method(virtual_machine, widget_class, "menu", c_widget_menu);
  mrbc_define_method(virtual_machine, widget_class, "alert", c_widget_alert);
}

static void
define_widget_list_methods(
  mrbc_vm *virtual_machine,
  mrbc_class *widget_list_class
) {

  mrbc_define_destructor(widget_list_class, free_widget_list);
  mrbc_define_method(
    virtual_machine,
    widget_list_class,
    "new",
    c_widget_list_new
  );
  mrbc_define_method(
    virtual_machine,
    widget_list_class,
    "area",
    c_widget_list_area
  );
  mrbc_define_method(
    virtual_machine,
    widget_list_class,
    "clear",
    c_widget_list_clear
  );
  mrbc_define_method(
    virtual_machine,
    widget_list_class,
    "add",
    c_widget_list_add
  );
  mrbc_define_method(
    virtual_machine,
    widget_list_class,
    "empty_text=",
    c_widget_list_set_empty_text
  );
  mrbc_define_method(
    virtual_machine,
    widget_list_class,
    "show_marks=",
    c_widget_list_set_show_marks
  );
  mrbc_define_method(
    virtual_machine,
    widget_list_class,
    "toggle_mark",
    c_widget_list_toggle_mark
  );
  mrbc_define_method(
    virtual_machine,
    widget_list_class,
    "mark",
    c_widget_list_mark
  );
  mrbc_define_method(
    virtual_machine,
    widget_list_class,
    "marked?",
    c_widget_list_marked
  );
  mrbc_define_method(
    virtual_machine,
    widget_list_class,
    "count",
    c_widget_list_count
  );
  mrbc_define_method(
    virtual_machine,
    widget_list_class,
    "index",
    c_widget_list_index
  );
  mrbc_define_method(
    virtual_machine,
    widget_list_class,
    "index=",
    c_widget_list_set_index
  );
  mrbc_define_method(
    virtual_machine,
    widget_list_class,
    "top",
    c_widget_list_top
  );
  mrbc_define_method(
    virtual_machine,
    widget_list_class,
    "handle",
    c_widget_list_handle
  );
  mrbc_define_method(
    virtual_machine,
    widget_list_class,
    "draw",
    c_widget_list_draw
  );
}

static void
define_widget_text_view_methods(
  mrbc_vm *virtual_machine,
  mrbc_class *widget_text_view_class
) {

  mrbc_define_destructor(widget_text_view_class, free_widget_text_view);
  mrbc_define_method(
    virtual_machine,
    widget_text_view_class,
    "new",
    c_widget_text_view_new
  );
  mrbc_define_method(
    virtual_machine,
    widget_text_view_class,
    "area",
    c_widget_text_view_area
  );
  mrbc_define_method(
    virtual_machine,
    widget_text_view_class,
    "text=",
    c_widget_text_view_set_text
  );
  mrbc_define_method(
    virtual_machine,
    widget_text_view_class,
    "scroll",
    c_widget_text_view_scroll
  );
  mrbc_define_method(
    virtual_machine,
    widget_text_view_class,
    "scroll=",
    c_widget_text_view_set_scroll
  );
  mrbc_define_method(
    virtual_machine,
    widget_text_view_class,
    "handle",
    c_widget_text_view_handle
  );
  mrbc_define_method(
    virtual_machine,
    widget_text_view_class,
    "draw",
    c_widget_text_view_draw
  );
}

void
mrbc_area512_widget_init(mrbc_vm *virtual_machine) {
  mrbc_class *widget_class =
    mrbc_define_class(virtual_machine, "Widget", mrbc_class_object);
  mrbc_class *widget_list_class =
    mrbc_define_class(virtual_machine, "WidgetList", mrbc_class_object);
  mrbc_class *widget_text_view_class =
    mrbc_define_class(virtual_machine, "WidgetTextView", mrbc_class_object);

  define_widget_methods(virtual_machine, widget_class);
  define_widget_list_methods(virtual_machine, widget_list_class);
  define_widget_text_view_methods(virtual_machine, widget_text_view_class);
}

#elif defined(PICORB_VM_MRUBY)

#include <mruby.h>

void
mrb_picoruby_area512_widget_gem_init(mrb_state *state) {
  (void)state;
}

void
mrb_picoruby_area512_widget_gem_final(mrb_state *state) {
  (void)state;
}

#endif
