#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void area512_console_poll(void);
int area512_console_getchar(void);
int area512_console_getch_block(void);
int area512_console_getch_timeout(int timeout_ms);

void area512_console_write(const void *buffer, int buffer_byte_count);
void area512_console_reset(void);
int area512_console_had_output(void);

#ifdef __cplusplus
}
#endif
