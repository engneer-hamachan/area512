#include "console.h"

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
