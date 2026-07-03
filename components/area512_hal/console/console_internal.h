#pragma once

#include <stdint.h>

bool area512_console_stdin_push(uint8_t character);
void area512_console_stdin_push_bytes(const char *bytes);
extern "C" void area512_input_init(void);
bool keyboard_init(void);
void keyboard_poll(void);
