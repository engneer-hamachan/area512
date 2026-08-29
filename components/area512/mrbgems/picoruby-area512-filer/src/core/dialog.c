#if defined(PICORB_VM_MRUBYC)

#include "area512_hal.h"
#include "core/filer.h"

#include <stdio.h>
#include <string.h>

static void
write_input_window(
  Filer *filer,
  const char *label,
  const char *input_buffer,
  int input_byte_count
) {

  int available_column_count = filer->columns - (int)strlen(label) - 1;
  int window_byte_offset;

  if (available_column_count < 1)
    available_column_count = 1;

  window_byte_offset = input_byte_count - available_column_count;

  if (window_byte_offset < 0)
    window_byte_offset = 0;

  snprintf(
    filer->message,
    MESSAGE_MAX,
    "%s%s_",
    label,
    input_buffer + window_byte_offset
  );
}

void
build_delete_question(
  Filer *filer,
  const char *target_label,
  int target_is_directory,
  char *question,
  int question_size
) {

  char fitted_label[NAME_MAX];

  fit_string(
    fitted_label,
    sizeof fitted_label,
    target_label,
    filer->columns - 22
  );

  if (target_is_directory)
    snprintf(
      question,
      question_size,
      "Delete %s/ & contents? (y/n)",
      fitted_label
    );
  else
    snprintf(question, question_size, "Delete %s? (y/n)", fitted_label);
}

// Reads a line of text into filer->input. Returns 1 on confirm, 0 on cancel
// or empty input.
int
read_text_input(Filer *filer, const char *label) {
  char input_buffer[LINE_MAX];
  int length = 0;

  input_buffer[0] = 0;

  for (;;) {
    write_input_window(filer, label, input_buffer, length);
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

    } else if (key >= ' ' && key <= '~' && length < LINE_MAX - 1) {
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
