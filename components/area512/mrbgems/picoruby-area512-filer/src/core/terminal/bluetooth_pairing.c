#if defined(PICORB_VM_MRUBYC)

#include "core/terminal/terminal.h"

#include "area512_hal.h"
#include <inttypes.h>
#include <stdio.h>

#define PAIRING_POLL_INTERVAL_MILLISECONDS 100

static int
read_found_keyboard_index(Filer *filer, int found_keyboard_count) {
  int input_key;

  for (;;) {
    draw_terminal(filer);

    input_key = read_terminal_key();

    if (input_key == 27)
      return -1;

    if (input_key >= '1' && input_key < '1' + found_keyboard_count)
      return input_key - '1';
  }
}

static void
show_pairing_progress(Filer *filer) {
  char line_text[LINE_MAX];
  int is_passkey_shown = 0;
  int pairing_state;

  for (;;) {
    pairing_state = area512_bluetooth_pairing_state();

    if (pairing_state == AREA512_BLUETOOTH_PAIRING_SUCCEEDED) {
      append_output_text(filer->terminal, "Paired");

      return;
    }

    if (pairing_state == AREA512_BLUETOOTH_PAIRING_FAILED) {
      snprintf(
        line_text,
        sizeof line_text,
        "Pairing failed: %s",
        area512_bluetooth_pairing_failure_message()
      );

      append_output_text(filer->terminal, line_text);

      return;
    }

    if (
      pairing_state == AREA512_BLUETOOTH_PAIRING_WAITING_FOR_PASSKEY_ENTRY &&
      !is_passkey_shown
    ) {

      snprintf(
        line_text,
        sizeof line_text,
        "Type %06" PRIu32 " + Enter on the keyboard",
        area512_bluetooth_pairing_passkey()
      );

      append_output_text(filer->terminal, line_text);

      is_passkey_shown = 1;
    }

    draw_terminal(filer);

    if (
      read_terminal_key_with_timeout(PAIRING_POLL_INTERVAL_MILLISECONDS) == 27
    ) {

      area512_bluetooth_cancel_pairing();
      append_output_text(filer->terminal, "Canceled");

      return;
    }
  }
}

void
pair_bluetooth_keyboard(Filer *filer) {
  char line_text[LINE_MAX];
  int found_keyboard_count;
  int found_keyboard_index;

  if (!area512_bluetooth_is_running()) {
    append_output_text(filer->terminal, "Bluetooth is off");

    return;
  }

  if (area512_bluetooth_is_keyboard_connected()) {
    append_output_text(filer->terminal, "Keyboard already connected");

    return;
  }

  append_output_text(filer->terminal, "Scanning...");
  draw_terminal(filer);

  found_keyboard_count = area512_bluetooth_scan_keyboards();

  if (found_keyboard_count <= 0) {
    area512_bluetooth_cancel_pairing();

    append_output_text(
      filer->terminal,
      found_keyboard_count < 0 ? "Scan failed" : "No keyboard found"
    );

    return;
  }

  for (int i = 0; i < found_keyboard_count; i++) {
    snprintf(
      line_text,
      sizeof line_text,
      "%d %s",
      i + 1,
      area512_bluetooth_found_keyboard_label(i)
    );

    append_output_text(filer->terminal, line_text);
  }

  snprintf(
    line_text,
    sizeof line_text,
    "Select 1-%d (Esc cancels)",
    found_keyboard_count
  );

  append_output_text(filer->terminal, line_text);

  found_keyboard_index = read_found_keyboard_index(filer, found_keyboard_count);

  if (found_keyboard_index < 0) {
    area512_bluetooth_cancel_pairing();
    append_output_text(filer->terminal, "Canceled");

    return;
  }

  area512_bluetooth_start_pairing(found_keyboard_index);
  append_output_text(filer->terminal, "Pairing...");

  show_pairing_progress(filer);

  if (!area512_bluetooth_save_bonds())
    append_output_text(filer->terminal, "Bond save failed");
}

#endif
