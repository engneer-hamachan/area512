#if defined(PICORB_VM_MRUBYC)

#include "area512_hal.h"
#include "area512_repl.h"

#include <string.h>

void
write_console_line(const char *text) {
  area512_console_write(text, strlen(text));
  area512_console_write("\n", 1);
}

void
echo_repl_input_line(const char *prompt, const ReplLine *repl_line) {
  area512_console_write(prompt, strlen(prompt));
  area512_console_write(repl_line->input_line, repl_line->input_byte_count);
  area512_console_write("\n", 1);
}

#endif
