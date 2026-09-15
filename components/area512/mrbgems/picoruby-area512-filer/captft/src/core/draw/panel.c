#include "core/draw/draw.h"

#include <stdio.h>
#include <string.h>

#define PANEL_STRIP_TOP \
  ((FRAME_TOP + BAR1_Y - PANEL_HEIGHT) / 2 - HEADER_HEIGHT)
#define PANEL_TITLE_TOP 8
#define PANEL_METRIC_TOP 24
#define PANEL_METRIC_HEIGHT 15
#define PANEL_BAR_HEIGHT 6

void
build_panel_info(PanelInfo *panel_information) {
  int battery = area512_metrics_battery_percent();
  panel_information->label[0] = "BAT";
  panel_information->percent[0] = battery;

  int virtual_machine_percent = area512_metrics_vm_used_pct();
  panel_information->label[1] = "VM";
  panel_information->percent[1] = virtual_machine_percent;

  int ram_percent = area512_metrics_dram_used_pct();
  panel_information->label[2] = "RAM";
  panel_information->percent[2] = ram_percent;

  int stack_percent = area512_metrics_stack_used_pct();
  panel_information->label[3] = "STK";
  panel_information->percent[3] = stack_percent;
  panel_information->count = 4;
}

static void
draw_meter(
  Filer *filer,
  const char *label,
  int percent,
  int meter_top_y
) {

  int label_x = PANEL_X + 8;
  int text_y = meter_top_y;

  area512_sprite_text(
    filer->row,
    label_x,
    text_y,
    label,
    area512_theme_text_color()
  );

  int value_right = PANEL_RIGHT - 8;
  int value_field_width = 4 * FILER_CHAR_WIDTH;
  int bar_x = label_x + 3 * FILER_CHAR_WIDTH + 4;
  int bar_end_x = value_right - value_field_width - 4;
  int bar_width = bar_end_x - bar_x;

  if (bar_width < 6)
    bar_width = 6;

  int bar_y = meter_top_y + (FILER_FONT_SIZE - PANEL_BAR_HEIGHT) / 2;
  uint32_t color = area512_theme_border_color();

  area512_sprite_line(
    filer->row,
    bar_x,
    bar_y,
    bar_x + bar_width,
    bar_y,
    area512_theme_border_color()
  );

  area512_sprite_line(
    filer->row,
    bar_x,
    bar_y + PANEL_BAR_HEIGHT,
    bar_x + bar_width,
    bar_y + PANEL_BAR_HEIGHT,
    area512_theme_border_color()
  );

  area512_sprite_line(
    filer->row,
    bar_x,
    bar_y,
    bar_x,
    bar_y + PANEL_BAR_HEIGHT,
    area512_theme_border_color()
  );

  area512_sprite_line(
    filer->row,
    bar_x + bar_width,
    bar_y,
    bar_x + bar_width,
    bar_y + PANEL_BAR_HEIGHT,
    area512_theme_border_color()
  );

  if (percent > 0) {
    int clamped_percent = percent > 100 ? 100 : percent;
    int fill_width = (bar_width - 2) * clamped_percent / 100;

    if (fill_width > 0)
      area512_sprite_fill_rect(
        filer->row,
        bar_x + 1,
        bar_y + 1,
        fill_width,
        PANEL_BAR_HEIGHT - 1,
        color
      );
  }

  char value_text[8];

  if (percent < 0) {
    value_text[0] = value_text[1] = '-';
    value_text[2] = 0;
  } else
    snprintf(value_text, sizeof value_text, "%d%%", percent);

  int value_text_width = (int)strlen(value_text) * FILER_CHAR_WIDTH;

  area512_sprite_text(
    filer->row,
    value_right - value_text_width,
    text_y,
    value_text,
    area512_theme_emphasis_color()
  );
}

static int
compute_panel_strip_offset(int row) {
  return row * ROW_HEIGHT - PANEL_STRIP_TOP;
}

void
draw_panel_row(Filer *filer, int row, const PanelInfo *panel_information) {
  if (!panel_covers_row(row))
    return;

  int offset = compute_panel_strip_offset(row);
  int left = PANEL_X;
  int width = PANEL_RIGHT - left + 1;
  int title_width = area512_sprite_text_width(filer->row, PANEL_TITLE);

  area512_sprite_rect(
    filer->row,
    left,
    -offset,
    width,
    PANEL_HEIGHT,
    area512_theme_border_color()
  );

  area512_sprite_text(
    filer->row,
    left + (width - title_width) / 2,
    PANEL_TITLE_TOP - offset,
    PANEL_TITLE,
    area512_theme_emphasis_color()
  );

  for (
    int metric_index = 0;
    metric_index < panel_information->count;
    ++metric_index
  ) {

    draw_meter(
      filer,
      panel_information->label[metric_index],
      panel_information->percent[metric_index],
      PANEL_METRIC_TOP + metric_index * PANEL_METRIC_HEIGHT - offset
    );
  }
}

int
panel_covers_row(int row) {
  int offset = compute_panel_strip_offset(row);

  return offset > -ROW_HEIGHT && offset < PANEL_HEIGHT;
}
