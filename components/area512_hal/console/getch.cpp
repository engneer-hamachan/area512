#include "console.h"

#if defined(AREA512_CYD)
#include "area512_hal.h"
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static int
poll_for_character(int timeout_ms) {
  int elapsed_ms = 0;

  for (;;) {
    int character = area512_console_getchar();

    if (character >= 0) {
      return character;
    }

    if (timeout_ms >= 0 && elapsed_ms >= timeout_ms) {
      return -1;
    }

    vTaskDelay(pdMS_TO_TICKS(2));

    elapsed_ms += 2;
  }
}

extern "C" int
area512_console_getch_block(void) {
  return poll_for_character(-1);
}

extern "C" int
area512_console_getch_timeout(int timeout_ms) {
  return poll_for_character(timeout_ms);
}

#if defined(AREA512_CYD)
static void
wait_for_character_or_touch_end(void) {
  int kind;
  int touch_screen_x;
  int touch_screen_y;
  int64_t event_uptime_ms;

  area512_clear_touch_events();

  for (;;) {
    if (area512_console_getchar() >= 0)
      return;

    if (
      area512_read_touch_event(
        &kind,
        &touch_screen_x,
        &touch_screen_y,
        &event_uptime_ms
      ) &&
      kind != AREA512_TOUCH_DRAG
    )
      return;

    vTaskDelay(pdMS_TO_TICKS(2));
  }
}
#endif

extern "C" void
area512_console_wait_key_if_output(void) {
  if (!area512_console_had_output())
    return;

#if defined(AREA512_CYD)
  wait_for_character_or_touch_end();
#else
  area512_console_getch_block();
#endif
}
