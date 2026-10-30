#pragma once

#include <stdint.h>

bool area512_console_stdin_push(uint8_t character);
void area512_console_stdin_push_bytes(const char *bytes);
void area512_console_feed_ime(uint8_t byte);
void area512_console_flush_ime(void);
void area512_console_toggle_ime(void);
extern "C" void area512_input_init(void);
bool keyboard_init(void);
void keyboard_poll(void);
void area512_bluetooth_poll_keyboard(void);
