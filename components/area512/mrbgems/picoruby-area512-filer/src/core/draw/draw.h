#pragma once

#include "area512_hal.h"
#include "core/filer.h"

#define PANEL_TITLE "SYS 0x200"
#define PANEL_DELTA_Y 3

typedef struct {
  int count;
  const char *label[PANEL_MAX_METRICS];
  int percent[PANEL_MAX_METRICS];
} PanelInfo;

void find_changed_rows(Filer *filer, const PanelInfo *info, uint8_t *rows);
void save_draw_state(Filer *filer, const PanelInfo *info);

void draw_frame(Filer *filer);
void draw_header(Filer *filer);

void build_panel_info(PanelInfo *panel_information);
void draw_panel_row(Filer *filer, int row, const PanelInfo *panel_information);
int panel_covers_row(Filer *filer, int row, const PanelInfo *panel_information);

void adjust_scroll(Filer *filer);
void draw_entry(Filer *filer, int y, int index, int row, const PanelInfo *info);

void draw_primary_action_bar(Filer *filer);
void draw_secondary_action_bar(Filer *filer);
