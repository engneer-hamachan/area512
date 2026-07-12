#if defined(PICORB_VM_MRUBYC)

#include "area512_hal.h"
#include "core/widget.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int
clamp(int value, int minimum, int maximum) {
  if (value < minimum)
    return minimum;

  if (value > maximum)
    return maximum;

  return value;
}

static void
copy_bounded(char *destination, int destination_size, const char *source) {
  int length = (int)strlen(source);

  if (length >= destination_size)
    length = destination_size - 1;

  memcpy(destination, source, length);
  destination[length] = 0;
}

static void
draw_input_panel(void *sprite, const char *label, const char *input) {
  int width = area512_gfx_width();
  int y = area512_gfx_height() - 50;
  int available_width = width - 8;
  char visible[WIDGET_INPUT_MAX + 2];
  char with_cursor[WIDGET_INPUT_MAX + 2];

  snprintf(with_cursor, sizeof with_cursor, "%s_", input);

  const char *start = with_cursor;

  while (*start && area512_widget_text_width(sprite, start) > available_width) {
    start++;

    while (*start && ((unsigned char)*start & 0xC0) == 0x80)
      start++;
  }

  copy_bounded(visible, sizeof visible, start);

  int two_line_height = WIDGET_ROW_HEIGHT + WIDGET_FONT_HEIGHT;
  int label_y = y + (36 - two_line_height) / 2;

  area512_widget_draw_panel(sprite, 0, y, width, 36);
  area512_sprite_text(sprite, 4, label_y, label, WIDGET_COLOR_DIM);
  area512_sprite_text(
    sprite,
    4,
    label_y + WIDGET_ROW_HEIGHT,
    visible,
    WIDGET_COLOR_GOLD
  );
  area512_sprite_push(sprite, 0, 0);
}

static int
run_input_loop(
  void *sprite,
  const char *label,
  char input[WIDGET_INPUT_MAX],
  int numeric
) {

  int length = (int)strlen(input);

  for (;;) {
    draw_input_panel(sprite, label, input);
    WidgetKey key = area512_widget_read_text_key();

    if (key.kind == WIDGET_KEY_ESCAPE)
      return 0;

    if (key.kind == WIDGET_KEY_ENTER)
      return 1;

    if (key.kind == WIDGET_KEY_BACKSPACE) {
      if (length > 0) {
        do {
          length--;
        } while (length > 0 && ((unsigned char)input[length] & 0xC0) == 0x80);

        input[length] = 0;
      }

      continue;
    }

    if (key.kind != WIDGET_KEY_BYTE || key.byte < ' ' || key.byte > '~')
      continue;

    if (numeric && !((key.byte >= '0' && key.byte <= '9') ||
                     (key.byte == '-' && length == 0)))
      continue;

    if (length < WIDGET_INPUT_MAX - 1) {
      input[length++] = key.byte;
      input[length] = 0;
    }
  }
}

int
area512_widget_run_input_modal(
  void *sprite,
  const char *label,
  const char *initial,
  char output[WIDGET_INPUT_MAX]
) {

  copy_bounded(output, WIDGET_INPUT_MAX, initial);
  return run_input_loop(sprite, label, output, 0);
}

int
area512_widget_run_number_modal(
  void *sprite,
  const char *label,
  int initial,
  int minimum,
  int maximum,
  int *result
) {

  char input[WIDGET_INPUT_MAX];

  snprintf(input, sizeof input, "%d", initial);

  if (!run_input_loop(sprite, label, input, 1))
    return 0;

  *result = clamp(atoi(input), minimum, maximum);
  return 1;
}

int
area512_widget_run_confirm_modal(void *sprite, const char *question) {
  for (;;) {
    draw_input_panel(sprite, question, "[y] Yes   [n] No");
    WidgetKey key = area512_widget_read_key();

    if (key.kind == WIDGET_KEY_ESCAPE)
      return 0;

    if (key.kind != WIDGET_KEY_BYTE)
      continue;

    if (key.byte == 'y' || key.byte == 'Y')
      return 1;

    if (key.byte == 'n' || key.byte == 'N')
      return 0;
  }
}

static int
panel_width_for_items(
  void *sprite,
  const char *title,
  const char *const *items,
  int item_count
) {

  int width = area512_widget_text_width(sprite, title) + 24;

  for (int i = 0; i < item_count; i++) {
    int item_width = area512_widget_text_width(sprite, items[i]) + 24;

    if (item_width > width)
      width = item_width;
  }

  return clamp(width, 96, area512_gfx_width() - 24);
}

int
area512_widget_run_dialog_modal(
  void *sprite,
  const char *message,
  const char *const *buttons,
  int button_count
) {

  if (button_count <= 0)
    return -1;

  int selected = 0;
  int panel_width =
    panel_width_for_items(sprite, message, buttons, button_count);
  int panel_x = (area512_gfx_width() - panel_width) / 2;
  int panel_y = (area512_gfx_height() - 44) / 2;
  int button_width = (panel_width - 12) / button_count;

  for (;;) {
    area512_widget_draw_titled_panel(
      sprite,
      panel_x,
      panel_y,
      panel_width,
      44,
      message
    );

    for (int i = 0; i < button_count; i++) {
      area512_widget_draw_button(
        sprite,
        panel_x + 6 + i * button_width,
        panel_y + 22,
        button_width - 2,
        buttons[i],
        i == selected
      );
    }

    area512_sprite_push(sprite, 0, 0);
    WidgetKey key = area512_widget_read_key();

    if (key.kind == WIDGET_KEY_ESCAPE)
      return -1;

    if (key.kind == WIDGET_KEY_ENTER)
      return selected;

    if (key.kind == WIDGET_KEY_LEFT)
      selected = clamp(selected - 1, 0, button_count - 1);

    if (key.kind == WIDGET_KEY_RIGHT)
      selected = clamp(selected + 1, 0, button_count - 1);
  }
}

int
area512_widget_run_menu_modal(
  void *sprite,
  const char *title,
  const char *const *items,
  int item_count,
  int initial
) {

  if (item_count <= 0)
    return -1;

  int selected = clamp(initial, 0, item_count - 1);
  int visible_count = (area512_gfx_height() - 34) / WIDGET_ROW_HEIGHT;

  if (visible_count > item_count)
    visible_count = item_count;

  int panel_width = panel_width_for_items(sprite, title, items, item_count);
  int panel_height = visible_count * WIDGET_ROW_HEIGHT + 18;
  int panel_x = (area512_gfx_width() - panel_width) / 2;
  int panel_y = (area512_gfx_height() - panel_height) / 2;

  for (;;) {
    int top = selected - visible_count + 1;

    if (top < 0)
      top = 0;

    area512_widget_draw_titled_panel(
      sprite,
      panel_x,
      panel_y,
      panel_width,
      panel_height,
      title
    );

    for (int row = 0; row < visible_count; row++) {
      int index = top + row;
      int row_y = panel_y + 16 + row * WIDGET_ROW_HEIGHT;

      if (index == selected) {
        area512_sprite_fill_rect(
          sprite,
          panel_x + 2,
          row_y,
          panel_width - 4,
          WIDGET_ROW_HEIGHT,
          WIDGET_COLOR_DARK
        );
      }

      area512_sprite_text(
        sprite,
        panel_x + 8,
        area512_widget_vcenter_text_y(row_y, WIDGET_ROW_HEIGHT),
        items[index],
        index == selected ? WIDGET_COLOR_GOLD : WIDGET_COLOR_DIM
      );
    }

    area512_widget_draw_scrollbar(
      sprite,
      panel_x + panel_width - 4,
      panel_y + 16,
      visible_count * WIDGET_ROW_HEIGHT,
      top,
      visible_count,
      item_count
    );

    area512_sprite_push(sprite, 0, 0);
    WidgetKey key = area512_widget_read_key();

    if (key.kind == WIDGET_KEY_ESCAPE)
      return -1;

    if (key.kind == WIDGET_KEY_ENTER)
      return selected;

    if (key.kind == WIDGET_KEY_UP ||
        (key.kind == WIDGET_KEY_BYTE && (key.byte == 'k' || key.byte == ';')))
      selected = clamp(selected - 1, 0, item_count - 1);

    if (key.kind == WIDGET_KEY_DOWN ||
        (key.kind == WIDGET_KEY_BYTE && (key.byte == 'j' || key.byte == '.')))
      selected = clamp(selected + 1, 0, item_count - 1);
  }
}

void
area512_widget_run_alert_modal(void *sprite, const char *message) {
  int panel_width = clamp(
    area512_widget_text_width(sprite, message) + 24,
    96,
    area512_gfx_width() - 24
  );
  int panel_x = (area512_gfx_width() - panel_width) / 2;
  int panel_y = (area512_gfx_height() - 44) / 2;

  int two_line_height = WIDGET_ROW_HEIGHT + WIDGET_FONT_HEIGHT;
  int message_y = panel_y + (44 - two_line_height) / 2;

  area512_widget_draw_panel(sprite, panel_x, panel_y, panel_width, 44);
  area512_widget_draw_text_center(
    sprite,
    message_y,
    message,
    WIDGET_COLOR_GOLD
  );
  area512_widget_draw_text_center(
    sprite,
    message_y + WIDGET_ROW_HEIGHT,
    "Press any key",
    WIDGET_COLOR_DIM
  );
  area512_sprite_push(sprite, 0, 0);
  area512_widget_read_key();
}

#endif
