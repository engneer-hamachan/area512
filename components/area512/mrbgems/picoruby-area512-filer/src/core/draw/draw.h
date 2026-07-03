#pragma once

#include "area512_hal.h"
#include "core/filer.h"

#define PANEL_TITLE "SYS 0x200"
#define PANEL_DELTA_Y 3
#define PANEL_LOW 20
#define PANEL_HIGH 80
#define PANEL_MAX_METRICS 4

typedef struct {
  int count;
  const char *label[PANEL_MAX_METRICS];
  int percent[PANEL_MAX_METRICS];
  int warn[PANEL_MAX_METRICS];
} PanelInfo;

void draw_walls(Filer *filer, int top);
void draw_header(Filer *filer);
void draw_close(Filer *filer);

void build_panel_info(PanelInfo *panel_information);
void draw_panel_row(Filer *filer, int row, const PanelInfo *panel_information);
int panel_covers_row(Filer *filer, int row, const PanelInfo *panel_information);

void adjust_scroll(Filer *filer);
void draw_entry(Filer *filer, int y, int index, int row, const PanelInfo *info);

void draw_primary_action_bar(Filer *filer);
void draw_secondary_action_bar(Filer *filer);
