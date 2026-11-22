#pragma once

#include "area512_hal.h"
#include "core/filer.h"

#define PANEL_TITLE "SYS 0x200"

#define WINDOW_LEFT ((SCREEN_WIDTH - AREA512_WINDOW_WIDTH) / 2)
#define WINDOW_TOP_WITHOUT_SOFT_KEYBOARD \
  ((FRAME_TOP + BAR_Y - AREA512_WINDOW_HEIGHT) / 2)
#define SOFT_KEYBOARD_MESSAGE_Y \
  (AREA512_SOFT_KEYBOARD_TOP - SOFT_KEYBOARD_MESSAGE_HEIGHT)
// The chip dividers reach below BAR_Y + BAR_HEIGHT.
#define BAR_TO_FRAME_BOTTOM_ROW_COUNT (CLOSE_Y + FRAME_BOTTOM_HEIGHT - BAR_Y)

int find_window_left(const Filer *filer);
int find_window_top(const Filer *filer);
int compute_corner_inset(int dy, int radius);

void find_changed_rows(Filer *filer, uint8_t *rows);
void save_draw_state(Filer *filer);

void draw_frame(Filer *filer);
void draw_header(Filer *filer);

void build_panel_info(PanelInfo *panel_information);
void draw_panel_row(Filer *filer, int row, const PanelInfo *panel_information);
int panel_covers_row(int row);

void adjust_scroll(Filer *filer);
void draw_entry(Filer *filer, int y, int index, int row, const PanelInfo *info);

void draw_action_bar(Filer *filer);
void draw_message_above_soft_keyboard(Filer *filer);
int is_action_bar_at(int touch_screen_y);
int find_action_bar_key_at(int touch_screen_x, int touch_screen_y);
int find_pressed_chip_index(Filer *filer);
