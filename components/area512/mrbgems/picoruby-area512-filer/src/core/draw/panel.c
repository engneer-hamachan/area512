#include "core/draw/draw.h"

#include <stdio.h>
#include <string.h>

void
build_panel_info(PanelInfo *panel_information) {
  int battery = area512_metrics_battery_percent();
  panel_information->label[0] = "BAT";
  panel_information->percent[0] = battery;
  panel_information->warn[0] = (battery >= 0 && battery <= PANEL_LOW);

  int virtual_machine_percent = area512_metrics_vm_used_pct();
  panel_information->label[1] = "VM";
  panel_information->percent[1] = virtual_machine_percent;
  panel_information->warn[1] = (virtual_machine_percent >= PANEL_HIGH);

  int ram_percent = area512_metrics_dram_used_pct();
  panel_information->label[2] = "RAM";
  panel_information->percent[2] = ram_percent;
  panel_information->warn[2] = (ram_percent >= PANEL_HIGH);

  int stack_percent = area512_metrics_stack_used_pct();
  panel_information->label[3] = "STK";
  panel_information->percent[3] = stack_percent;
  panel_information->warn[3] = (stack_percent >= PANEL_HIGH);
  panel_information->count = 4;
}

// meter_top_y may be negative: the sprite clips it and the neighboring strip
// draws the remainder, so the box can straddle a row boundary.
static void
draw_meter(
  Filer *filer,
  const char *label,
  int percent,
  int warn,
  int meter_top_y
) {

  int label_x = filer->panel_x + FILER_CHAR_WIDTH;

  area512_sprite_text(filer->row, label_x, meter_top_y, label, COLOR_DIM);

  // Fixed bar/value layout so the bar length doesn't shift with digit count.
  int value_right = filer->panel_right - 5;
  int value_field_width = 4 * FILER_CHAR_WIDTH;
  int bar_x = label_x + 3 * FILER_CHAR_WIDTH + 4;
  int bar_end_x = value_right - value_field_width - 4;
  int bar_width = bar_end_x - bar_x;

  if (bar_width < 6)
    bar_width = 6;

  int bar_y = meter_top_y + 3, bar_height = 6;
  uint32_t color = warn ? COLOR_AMBER : COLOR_GREEN;

  area512_sprite_line(
    filer->row,
    bar_x,
    bar_y,
    bar_x + bar_width,
    bar_y,
    COLOR_FAINT
  );

  area512_sprite_line(
    filer->row,
    bar_x,
    bar_y + bar_height,
    bar_x + bar_width,
    bar_y + bar_height,
    COLOR_FAINT
  );

  area512_sprite_line(
    filer->row,
    bar_x,
    bar_y,
    bar_x,
    bar_y + bar_height,
    COLOR_FAINT
  );

  area512_sprite_line(
    filer->row,
    bar_x + bar_width,
    bar_y,
    bar_x + bar_width,
    bar_y + bar_height,
    COLOR_FAINT
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
        bar_height - 1,
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
    meter_top_y,
    value_text,
    color
  );
}

static int
compute_panel_box_height(const PanelInfo *panel_information) {
  return (panel_information->count + 1) * ROW_HEIGHT;
}

static int
compute_panel_strip_offset(Filer *filer, int row) {
  return (row - filer->panel_top_row) * ROW_HEIGHT + PANEL_DELTA_Y;
}

// The box is one absolute-px rectangle sliced per strip at -offset, so it can
// straddle row boundaries and be shifted by PANEL_DELTA_Y.
void
draw_panel_row(Filer *filer, int row, const PanelInfo *panel_information) {
  if (!panel_covers_row(filer, row, panel_information))
    return;

  int box_height = compute_panel_box_height(panel_information);
  int offset = compute_panel_strip_offset(filer, row);
  int left_x = filer->panel_x, right_x = filer->panel_right;

  area512_sprite_line(
    filer->row,
    left_x,
    -offset,
    left_x,
    box_height - 1 - offset,
    COLOR_GREEN
  );

  area512_sprite_line(
    filer->row,
    right_x,
    -offset,
    right_x,
    box_height - 1 - offset,
    COLOR_GREEN
  );

  area512_sprite_line(filer->row, left_x, -offset, right_x, -offset, COLOR_GREEN);

  {
    const char *title = PANEL_TITLE;
    int title_width = (int)strlen(title) * FILER_CHAR_WIDTH;
    int title_x = left_x + (right_x - left_x - title_width) / 2;

    area512_sprite_text(
      filer->row,
      title_x,
      1 - offset,
      title,
      COLOR_GREEN
    );
  }

  for (int metric_index = 0; metric_index < panel_information->count;
       ++metric_index) {

    int metric_y = (metric_index + 1) * ROW_HEIGHT - offset;

    if (metric_y > -ROW_HEIGHT && metric_y < ROW_HEIGHT) {
      draw_meter(
        filer,
        panel_information->label[metric_index],
        panel_information->percent[metric_index],
        panel_information->warn[metric_index],
        metric_y
      );
    }
  }

  area512_sprite_line(
    filer->row,
    left_x,
    box_height - 1 - offset,
    right_x,
    box_height - 1 - offset,
    COLOR_GREEN
  );
}

// Used to clip list text drawn behind the panel.
int
panel_covers_row(Filer *filer, int row, const PanelInfo *panel_information) {
  int box_height = compute_panel_box_height(panel_information);
  int offset = compute_panel_strip_offset(filer, row);

  return offset > -ROW_HEIGHT && offset < box_height;
}
