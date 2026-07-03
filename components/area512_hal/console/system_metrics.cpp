#include "system_metrics.h"

#include <M5Unified.hpp>

#include <stddef.h>

#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

extern "C" size_t picoruby_esp32_vm_heap_free_size(void);
extern "C" size_t picoruby_esp32_vm_heap_total_size(void);

static int
area512_metrics_battery_mv_to_percent(int mv) {
  static const struct {
    int mv;
    int pct;
  } table[] = {
    {4190, 100},
    {4050, 90},
    {3990, 80},
    {3890, 70},
    {3800, 60},
    {3720, 50},
    {3630, 40},
    {3530, 30},
    {3420, 20},
    {3300, 10},
    {3100, 0},
  };

  constexpr int n = sizeof(table) / sizeof(table[0]);

  if (mv >= table[0].mv)
    return 100;

  if (mv <= table[n - 1].mv)
    return 0;

  for (int i = 0; i < n - 1; ++i) {
    int hi_mv = table[i].mv;
    int lo_mv = table[i + 1].mv;

    if (mv <= hi_mv && mv >= lo_mv) {
      int hi_pct = table[i].pct;
      int lo_pct = table[i + 1].pct;

      return lo_pct + (hi_pct - lo_pct) * (mv - lo_mv) / (hi_mv - lo_mv);
    }
  }

  return 0;
}

static int
area512_metrics_battery_mv(void) {
  int mv = (int)M5.Power.getBatteryVoltage();
  return mv > 0 ? mv : -1;
}

extern "C" int
area512_metrics_battery_percent(void) {
  int mv = area512_metrics_battery_mv();
  return mv < 0 ? -1 : area512_metrics_battery_mv_to_percent(mv);
}

extern "C" int
area512_metrics_vm_used_pct(void) {
  size_t total = picoruby_esp32_vm_heap_total_size();

  if (total == 0)
    return -1;

  size_t freeb = picoruby_esp32_vm_heap_free_size();
  size_t used = total > freeb ? total - freeb : 0;

  return (int)(used * 100 / total);
}

extern "C" int
area512_metrics_dram_used_pct(void) {
  size_t total =
    heap_caps_get_total_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

  if (total == 0)
    return -1;

  size_t freeb = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  size_t used = total > freeb ? total - freeb : 0;

  return (int)(used * 100 / total);
}

extern "C" int
area512_metrics_stack_used_pct(void) {
  unsigned total = (unsigned)CONFIG_ESP_MAIN_TASK_STACK_SIZE;

  if (total == 0)
    return -1;

  UBaseType_t free_min = uxTaskGetStackHighWaterMark(NULL);

  unsigned used = total > (unsigned)free_min ? total - (unsigned)free_min : 0;

  return (int)((unsigned long)used * 100 / total);
}
