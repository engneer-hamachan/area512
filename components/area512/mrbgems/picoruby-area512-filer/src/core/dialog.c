#if defined(PICORB_VM_MRUBYC)

#include "core/filer.h"
#include "area512_hal.h"

#include <stdio.h>
#include <string.h>

// Reads a line of text into filer->input. Returns 1 on confirm, 0 on cancel
// or empty input.
int
read_text_input(Filer *filer, const char *label) {
  char input_buffer[NAME_MAX];
  int length = 0;

  input_buffer[0] = 0;

  for (;;) {
    snprintf(filer->message, MESSAGE_MAX, "%s%s_", label, input_buffer);
    draw_all(filer);

    int key = read_raw_text_key();

    if (key == '\r' || key == '\n') {
      filer->message[0] = 0;

      if (length == 0)
        return 0;

      memcpy(filer->input, input_buffer, length + 1);

      return 1;

    } else if (key == 27) {
      filer->message[0] = 0;

      return 0;

    } else if (key == '\b' || key == 127) {
      if (length > 0) {
        length--;
        input_buffer[length] = 0;
      }

    } else if (key >= ' ' && key <= '~' && length < NAME_MAX - 1) {
      input_buffer[length++] = (char)key;
      input_buffer[length] = 0;
    }
  }
}

// Reads a yes/no answer. Returns 1 on yes, 0 on no or ESC.
int
read_yes_no_confirmation(Filer *filer, const char *question) {
  strncpy(filer->message, question, MESSAGE_MAX - 1);
  filer->message[MESSAGE_MAX - 1] = 0;
  draw_all(filer);

  for (;;) {
    int key = area512_filer_read_key();

    if (key == 'y' || key == 'Y') {
      filer->message[0] = 0;
      return 1;
    }

    if (key == 'n' || key == 'N' || key == 27) {
      filer->message[0] = 0;
      return 0;
    }
  }
}

#endif
