#include "system_metrics.h"

#include "area512_task_replacement.h"

#if !defined(AREA512_TDECK) && !defined(AREA512_CYD_ES3C28P) &&                \
  !defined(AREA512_CYD_2432S028)
#include <M5Unified.hpp>
#endif

#include <stddef.h>
#include <stdint.h>

#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#if defined(AREA512_TDECK)
// Use the same ADC handles as Ruby and MicroPython.
extern "C" int ADC_init(uint8_t pin);
extern "C" uint32_t ADC_read_raw(uint8_t pin);

static constexpr uint8_t TDECK_BATTERY_ADC_PIN = 4;
static bool is_battery_adc_initialized = false;
#elif defined(AREA512_CYD_ES3C28P)
extern "C" int ADC_init(uint8_t pin);
extern "C" uint32_t ADC_read_raw(uint8_t pin);

static constexpr uint8_t ES3C28P_BATTERY_ADC_PIN = 9;
static bool is_battery_adc_initialized = false;
#endif

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
#if defined(AREA512_TDECK)
  if (!is_battery_adc_initialized) {
    if (ADC_init(TDECK_BATTERY_ADC_PIN) < 0)
      return -1;

    is_battery_adc_initialized = true;
  }

  uint32_t raw = ADC_read_raw(TDECK_BATTERY_ADC_PIN);

  if (raw == 0)
    return -1;

  // R2P2-ESP32 uses raw * 3.3 / 4095 volts and a 2:1 battery divider.
  return (int)((raw * 6600 + 4095 / 2) / 4095);
#elif defined(AREA512_CYD_ES3C28P)
  if (!is_battery_adc_initialized) {
    if (ADC_init(ES3C28P_BATTERY_ADC_PIN) < 0)
      return -1;

    is_battery_adc_initialized = true;
  }

  uint32_t raw = ADC_read_raw(ES3C28P_BATTERY_ADC_PIN);

  if (raw == 0)
    return -1;

  return (int)((raw * 6600 + 4095 / 2) / 4095);
#elif defined(AREA512_CYD_2432S028)
  return -1;
#else
  int mv = (int)M5.Power.getBatteryVoltage();
  return mv > 0 ? mv : -1;
#endif
}

extern "C" int
area512_metrics_battery_percent(void) {
  int mv = area512_metrics_battery_mv();
  return mv < 0 ? -1 : area512_metrics_battery_mv_to_percent(mv);
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
area512_metrics_dram_free_byte_size(void) {
  return (int)heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
}

extern "C" int
area512_metrics_stack_free_byte_size(void) {
  return (int)uxTaskGetStackHighWaterMark(NULL);
}

extern "C" int
area512_metrics_stack_used_pct(void) {
  unsigned total = (unsigned)AREA512_SHARED_STACK_BYTE_SIZE;

  UBaseType_t free_min = uxTaskGetStackHighWaterMark(NULL);

  unsigned used = total > (unsigned)free_min ? total - (unsigned)free_min : 0;

  return (int)((unsigned long)used * 100 / total);
}
