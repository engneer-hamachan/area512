#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Ruby-visible paths are rooted here: "/" in Ruby means this directory.
#define AREA512_DATA_ROOT "/sdcard/Area512_data"
#define AREA512_PATH_MAX 256

int
area512_resolve_data_path(const char *path, char *buffer, size_t buffer_size);

void area512_console_poll(void);
int area512_console_getchar(void);
int area512_console_getch_block(void);
int area512_console_getch_timeout(int timeout_ms);

void area512_console_write(const void *buffer, int buffer_byte_count);
void area512_console_reset(void);
int area512_console_had_output(void);

int area512_metrics_battery_percent(void);
int area512_metrics_vm_used_pct(void);
int area512_metrics_dram_used_pct(void);
int area512_metrics_stack_used_pct(void);

void *area512_sprite_new(int w, int h);
void *area512_sprite_new_with_font_size(int w, int h, int font_size);
void area512_sprite_set_font_size(void *p, int font_size);
int area512_sprite_font_height(int font_size);
void area512_sprite_delete(void *p);
int area512_sprite_width(void *p);
int area512_sprite_height(void *p);
void area512_sprite_fill(void *p, uint32_t color);
void area512_sprite_pixel(void *p, int x, int y, uint32_t color);
void
area512_sprite_line(void *p, int x0, int y0, int x1, int y1, uint32_t color);
void area512_sprite_rect(void *p, int x, int y, int w, int h, uint32_t color);
void
area512_sprite_fill_rect(void *p, int x, int y, int w, int h, uint32_t color);
void area512_sprite_circle(void *p, int x, int y, int r, uint32_t color);
void area512_sprite_fill_circle(void *p, int x, int y, int r, uint32_t color);
void
area512_sprite_text(void *p, int x, int y, const char *str, uint32_t color);
int area512_sprite_text_width(void *p, const char *str);
void area512_sprite_push(void *p, int x, int y);
void area512_sprite_push_transparent(void *p, int x, int y, uint32_t transp);

int area512_gfx_width(void);
int area512_gfx_height(void);
void area512_gfx_fill_screen(uint32_t color);
void area512_gfx_set_brightness(int brightness);
int area512_gfx_show_header_image(const char *path, int hold_milliseconds);

int area512_sd_mount(const char *base_path);
int area512_sd_unmount(void);
int area512_sd_mounted(void);
int area512_seed_restore(void);

#ifdef __cplusplus
}
#endif
