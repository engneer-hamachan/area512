#include "area512_hal.h"
#include "console_internal.h"

void
area512_bluetooth_load_config(void) {
}

void
area512_bluetooth_start_if_enabled(void) {
}

bool
area512_bluetooth_save_bonds(void) {
  return false;
}

bool
area512_bluetooth_is_running(void) {
  return false;
}

bool
area512_bluetooth_is_keyboard_connected(void) {
  return false;
}

int
area512_bluetooth_scan_keyboards(void) {
  return 0;
}

const char *
area512_bluetooth_found_keyboard_label(int found_keyboard_index) {
  return "";
}

void
area512_bluetooth_start_pairing(int found_keyboard_index) {
}

void
area512_bluetooth_cancel_pairing(void) {
}

int
area512_bluetooth_pairing_state(void) {
  return AREA512_BLUETOOTH_PAIRING_INACTIVE;
}

uint32_t
area512_bluetooth_pairing_passkey(void) {
  return 0;
}

const char *
area512_bluetooth_pairing_failure_message(void) {
  return "";
}

void
area512_bluetooth_poll_keyboard(void) {
}
