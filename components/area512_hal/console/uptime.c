#include "area512_hal.h"

#include "esp_timer.h"

uint32_t
area512_uptime_milliseconds(void) {
  return (uint32_t)(esp_timer_get_time() / 1000);
}
