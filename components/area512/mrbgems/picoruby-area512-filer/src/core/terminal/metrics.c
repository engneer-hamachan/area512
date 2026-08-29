#if defined(PICORB_VM_MRUBYC)

#include "area512_hal.h"
#include "core/terminal/terminal.h"

#include <stdio.h>

#define METRIC_BAR_BYTE_COUNT 20

static void
build_metric_bar_text(int metric_percent, char *bar_text) {
  int clamped_metric_percent = metric_percent;
  int filled_bar_byte_count;
  int bar_byte_index = 0;

  if (clamped_metric_percent < 0)
    clamped_metric_percent = 0;

  if (clamped_metric_percent > 100)
    clamped_metric_percent = 100;

  filled_bar_byte_count =
    clamped_metric_percent * METRIC_BAR_BYTE_COUNT / 100;

  while (bar_byte_index < METRIC_BAR_BYTE_COUNT) {
    bar_text[bar_byte_index] =
      bar_byte_index < filled_bar_byte_count ? '#' : '-';

    bar_byte_index++;
  }

  bar_text[METRIC_BAR_BYTE_COUNT] = 0;
}

static void
append_metric_line(
  Filer *filer,
  const char *metric_label,
  int metric_percent
) {

  char bar_text[METRIC_BAR_BYTE_COUNT + 1];
  char value_text[8];
  char line_text[LINE_MAX];

  build_metric_bar_text(metric_percent, bar_text);

  if (metric_percent < 0)
    snprintf(value_text, sizeof value_text, "--");
  else
    snprintf(value_text, sizeof value_text, "%d%%", metric_percent);

  snprintf(
    line_text,
    sizeof line_text,
    "%-3s [%s] %4s",
    metric_label,
    bar_text,
    value_text
  );

  append_output_text(filer->terminal, line_text);
}

void
show_metrics(Filer *filer) {
  append_metric_line(filer, "BAT", area512_metrics_battery_percent());
  append_metric_line(filer, "VM", area512_metrics_vm_used_pct());
  append_metric_line(filer, "RAM", area512_metrics_dram_used_pct());
  append_metric_line(filer, "STK", area512_metrics_stack_used_pct());
}

#endif
