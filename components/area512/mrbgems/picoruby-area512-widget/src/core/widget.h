// VM-independent declarations for the Area512 widget toolkit.
#pragma once

#include <stdint.h>

#define WIDGET_COLOR_BG 0x000000
#define WIDGET_COLOR_AMBER 0xF5972D
#define WIDGET_COLOR_DIM 0xCFA45F
#define WIDGET_COLOR_GOLD 0xFFD966
#define WIDGET_COLOR_DARK 0x241604

#define WIDGET_CHAR_WIDTH 6
#define WIDGET_FONT_HEIGHT 13
#define WIDGET_ROW_HEIGHT 16
#define WIDGET_HEADER_HEIGHT 17
#define WIDGET_FOOTER_HEIGHT 14

#define WIDGET_LIST_MAX_ROWS 128
#define WIDGET_LIST_TEXT_MAX 40
#define WIDGET_LIST_TAG_MAX 12
#define WIDGET_TEXT_VIEW_MAX 2048
#define WIDGET_INPUT_MAX 64
#define WIDGET_MENU_MAX_ITEMS 12
#define WIDGET_MENU_ITEM_MAX 32
#define WIDGET_TABLE_MAX_COLUMNS 8
#define WIDGET_CHART_MAX_VALUES 64

typedef enum {
  WIDGET_KEY_BYTE,
  WIDGET_KEY_UP,
  WIDGET_KEY_DOWN,
  WIDGET_KEY_LEFT,
  WIDGET_KEY_RIGHT,
  WIDGET_KEY_ENTER,
  WIDGET_KEY_ESCAPE,
  WIDGET_KEY_BACKSPACE
} WidgetKeyKind;

typedef struct {
  WidgetKeyKind kind;
  char byte;
} WidgetKey;

typedef struct {
  const char *key;
  const char *label;
} WidgetHint;

typedef struct {
  const char *text;
  uint32_t color;
} WidgetColoredLine;

typedef struct {
  int x;
  int y;
  int w;
  int h;
  int index;
  int top;
  int count;
  uint8_t show_marks;
  uint8_t marks[WIDGET_LIST_MAX_ROWS];
  char empty_text[WIDGET_LIST_TEXT_MAX];
  char texts[WIDGET_LIST_MAX_ROWS][WIDGET_LIST_TEXT_MAX];
  char tags[WIDGET_LIST_MAX_ROWS][WIDGET_LIST_TAG_MAX];
} WidgetList;

typedef struct {
  int x;
  int y;
  int w;
  int h;
  int scroll;
  char text[WIDGET_TEXT_VIEW_MAX];
} WidgetTextView;

int area512_widget_body_top(void);
int area512_widget_body_bottom(void);
int area512_widget_body_height(void);
int area512_widget_vcenter_text_y(int frame_y, int frame_height);
int area512_widget_text_width(void *sprite, const char *text);
void area512_widget_clip(
  void *sprite,
  const char *text,
  int width,
  char *output,
  int output_size
);

WidgetKey area512_widget_read_key(void);
WidgetKey area512_widget_read_text_key(void);
const char *area512_widget_key_name(WidgetKey key, char byte_string[2]);

void area512_widget_draw_header(
  void *sprite,
  const char *title,
  const char *right_text
);
void area512_widget_draw_footer(void *sprite, const char *message);
void area512_widget_draw_hints(
  void *sprite,
  const WidgetHint *hints,
  int hint_count
);
void area512_widget_draw_separator(void *sprite, int y);
void
area512_widget_draw_vertical_separator(void *sprite, int x, int y, int height);
void area512_widget_draw_tabs(
  void *sprite,
  int y,
  const char *const *labels,
  int label_count,
  int active_index
);

void area512_widget_draw_panel(void *sprite, int x, int y, int w, int h);
void area512_widget_draw_titled_panel(
  void *sprite,
  int x,
  int y,
  int w,
  int h,
  const char *title
);
void area512_widget_draw_toast(void *sprite, const char *message);
void area512_widget_draw_splash(
  void *sprite,
  const char *title,
  const char *subtitle
);

void area512_widget_draw_text_center(
  void *sprite,
  int y,
  const char *text,
  uint32_t color
);
void area512_widget_draw_text_right(
  void *sprite,
  int right_x,
  int y,
  const char *text,
  uint32_t color
);
void area512_widget_draw_center_lines(
  void *sprite,
  const WidgetColoredLine *lines,
  int line_count
);
int area512_widget_draw_wrap_text(
  void *sprite,
  int x,
  int y,
  int w,
  int h,
  const char *text,
  uint32_t color
);
int area512_widget_draw_marquee(
  void *sprite,
  int x,
  int y,
  int w,
  const char *text,
  int offset
);
void area512_widget_draw_big_text(
  void *sprite,
  int x,
  int y,
  const char *text,
  uint32_t color
);

void area512_widget_draw_cell(
  void *sprite,
  int x,
  int y,
  int w,
  int h,
  const char *text,
  int selected
);
void area512_widget_draw_table_header(
  void *sprite,
  int x,
  int y,
  const int *widths,
  const char *const *labels,
  int column_count
);
void area512_widget_draw_table_row(
  void *sprite,
  int x,
  int y,
  const int *widths,
  const char *const *values,
  int column_count,
  int selected
);
void area512_widget_draw_field(
  void *sprite,
  int x,
  int y,
  int w,
  const char *label,
  const char *value,
  int focused
);

void area512_widget_draw_gauge(
  void *sprite,
  int x,
  int y,
  int w,
  int value,
  int maximum
);
void area512_widget_draw_slider(
  void *sprite,
  int x,
  int y,
  int w,
  int value,
  int maximum,
  int focused
);
void area512_widget_draw_scrollbar(
  void *sprite,
  int x,
  int y,
  int h,
  int top,
  int visible,
  int total
);
void area512_widget_draw_horizontal_scrollbar(
  void *sprite,
  int x,
  int y,
  int w,
  int left,
  int visible,
  int total
);
void area512_widget_draw_badge(void *sprite, int x, int y, const char *text);
void area512_widget_draw_battery(void *sprite, int x, int y);
void area512_widget_draw_busy(void *sprite, int x, int y, int frame);
void area512_widget_draw_page_dots(void *sprite, int y, int count, int active);

void area512_widget_draw_bar_chart(
  void *sprite,
  int x,
  int y,
  int w,
  int h,
  const int *values,
  int value_count,
  int maximum
);
void area512_widget_draw_line_chart(
  void *sprite,
  int x,
  int y,
  int w,
  int h,
  const int *values,
  int value_count,
  int maximum
);

void area512_widget_draw_button(
  void *sprite,
  int x,
  int y,
  int w,
  const char *label,
  int focused
);
void area512_widget_draw_checkbox(
  void *sprite,
  int x,
  int y,
  const char *label,
  int checked,
  int focused
);
void area512_widget_draw_radio(
  void *sprite,
  int x,
  int y,
  const char *label,
  int selected,
  int focused
);
void
area512_widget_draw_toggle(void *sprite, int x, int y, int on, int focused);
void area512_widget_draw_spinner(
  void *sprite,
  int x,
  int y,
  int w,
  const char *text,
  int focused
);

int area512_widget_run_input_modal(
  void *sprite,
  const char *label,
  const char *initial,
  char output[WIDGET_INPUT_MAX]
);
int area512_widget_run_number_modal(
  void *sprite,
  const char *label,
  int initial,
  int minimum,
  int maximum,
  int *result
);
int area512_widget_run_confirm_modal(void *sprite, const char *question);
int area512_widget_run_dialog_modal(
  void *sprite,
  const char *message,
  const char *const *buttons,
  int button_count
);
int area512_widget_run_menu_modal(
  void *sprite,
  const char *title,
  const char *const *items,
  int item_count,
  int initial
);
void area512_widget_run_alert_modal(void *sprite, const char *message);

void area512_widget_list_init(WidgetList *list);
void area512_widget_list_clear(WidgetList *list);
void
area512_widget_list_add(WidgetList *list, const char *text, const char *tag);
void area512_widget_list_set_index(WidgetList *list, int index);
int area512_widget_list_handle(WidgetList *list, const char *key);
void area512_widget_list_draw(void *sprite, const WidgetList *list);

void area512_widget_text_view_init(WidgetTextView *view);
void area512_widget_text_view_set_text(WidgetTextView *view, const char *text);
void area512_widget_text_view_set_scroll(
  void *sprite,
  WidgetTextView *view,
  int scroll
);
int area512_widget_text_view_handle(
  void *sprite,
  WidgetTextView *view,
  const char *key
);
void area512_widget_text_view_draw(void *sprite, const WidgetTextView *view);
