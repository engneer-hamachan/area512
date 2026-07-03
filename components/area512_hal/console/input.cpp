#include "console.h"
#include "console_internal.h"

#include <stdint.h>

// Declared locally: area512_hal cannot include the machine port's header, but
// the linker resolves these across components.
extern "C" bool picorb_hal_stdin_push(uint8_t character);
extern "C" int picorb_hal_getchar(void);

static bool s_keyboard_ready = false;

extern "C" void
area512_input_init(void) {
  s_keyboard_ready = keyboard_init();
}

bool
area512_console_stdin_push(uint8_t character) {
  return picorb_hal_stdin_push(character);
}

void
area512_console_stdin_push_bytes(const char *bytes) {
  while (*bytes)
    picorb_hal_stdin_push((uint8_t)*bytes++);
}

extern "C" void
area512_console_poll(void) {
  if (s_keyboard_ready)
    keyboard_poll();
}

extern "C" int
area512_console_getchar(void) {
  return picorb_hal_getchar();
}
