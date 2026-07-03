#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void area512_console_poll(void);
int area512_console_getchar(void);
int area512_console_getch_block(void);
int area512_console_getch_timeout(int timeout_ms);

#ifdef __cplusplus
}
#endif
