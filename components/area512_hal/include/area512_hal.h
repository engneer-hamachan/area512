#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Ruby-visible paths are rooted here: "/" in Ruby means this directory.
#define AREA512_ROOT_PATH "/sdcard/Area512_data"
#define AREA512_PATH_MAX 256

int
area512_resolve_data_path(
  const char *path,
  char *resolved_path,
  size_t resolved_path_size
);

void area512_console_poll(void);
int area512_console_getchar(void);
int area512_console_getch_block(void);
int area512_console_getch_timeout(int timeout_ms);

void area512_console_write(const void *buffer, int buffer_byte_count);
void area512_console_reset(void);
int area512_console_had_output(void);
void area512_console_wait_key_if_output(void);
int area512_console_cursor_row_index(void);
int area512_console_row_height(void);
int area512_console_font_size(void);

int area512_metrics_battery_percent(void);
int area512_metrics_dram_used_pct(void);
int area512_metrics_stack_used_pct(void);
int area512_metrics_dram_free_byte_size(void);
int area512_metrics_stack_free_byte_size(void);

void area512_start_filer_cursor_animation(void);
void area512_draw_filer_cursor_animation(
  void *sprite,
  int x,
  int y,
  uint32_t color
);
void area512_start_filer_terminal_cursor_animation(void);
void area512_draw_filer_terminal_cursor_animation(
  void *sprite,
  int x,
  int y,
  uint32_t color
);
void area512_start_filer_window_open_animation(
  void (*draw_screen)(void *context),
  void *context
);
bool area512_is_filer_window_open_animation_running(void);
void area512_draw_filer_window_open_animation(
  void *sprite,
  int left,
  int top,
  int width,
  int height
);
void area512_start_filer_window_close_animation(
  void (*draw_screen)(void *context),
  void *context
);
bool area512_is_filer_window_close_animation_running(void);
void area512_draw_filer_window_close_animation(
  void *sprite,
  int left,
  int top,
  int width,
  int height
);

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
void area512_sprite_blend_rect(
  void *p,
  int x,
  int y,
  int w,
  int h,
  uint32_t color,
  int opacity
);
void area512_sprite_circle(void *p, int x, int y, int r, uint32_t color);
void area512_sprite_fill_circle(void *p, int x, int y, int r, uint32_t color);
void
area512_sprite_text(void *p, int x, int y, const char *str, uint32_t color);
int area512_sprite_text_width(void *p, const char *str);
void area512_sprite_push(void *p, int x, int y);
void area512_sprite_push_transparent(void *p, int x, int y, uint32_t transp);

#define AREA512_SCREEN_REGION_ROW_COUNT 32

// One sprite draws the whole screen a band of rows at a time, reusing the
// screen pixel buffer. Drawing between area512_screen_begin_region() and
// area512_screen_push_region() uses screen coordinates; rows outside the band
// are clipped away.
void *area512_screen_new(int font_size);
void area512_screen_draw_sprite(void *screen, void *sprite, int x, int y);
void area512_screen_draw_sprite_clipped(
  void *screen,
  void *sprite,
  int x,
  int y,
  int clip_x,
  int clip_y,
  int clip_width,
  int clip_height
);
void area512_screen_read_sprite(void *screen, void *sprite, int x, int y);
int area512_screen_draw_rgb565(
  void *screen,
  const char *path,
  char *error,
  size_t error_size
);
// Transfer the previous region, then prepare the next; false after the final transfer.
int area512_screen_draw(void *p);
int area512_screen_begin_region(void *p);
int area512_screen_begin_region_at(void *screen, int first_row, int row_count);
int area512_screen_region_top(void *p);
int area512_screen_region_bottom(void *p);
void area512_screen_push_region(void *p);

int area512_gfx_width(void);
int area512_gfx_height(void);
// The physical display, ignoring any window the filer has set for the app.
int area512_gfx_screen_width(void);
int area512_gfx_screen_height(void);
// Let the running app draw on the whole display. The filer sets its window
// again when the app returns. Does nothing when no window is set.
void area512_gfx_full_screen(void);
void area512_gfx_fill_screen(uint32_t color);
void area512_gfx_set_brightness(int brightness);
int area512_gfx_show_header_image(const char *path, int hold_milliseconds);
int area512_gfx_load_header_image(
  const char *path,
  uint8_t *bitmap,
  size_t bitmap_size
);
void area512_gfx_draw_theme_bitmap(
  int x,
  int y,
  const uint8_t *bitmap,
  int w,
  int h
);

int area512_sd_mount(const char *base_path);
int area512_sd_unmount(void);
int area512_sd_mounted(void);
int area512_seed_restore(void);
int area512_seed_update(void);
const uint8_t *area512_seed_find_file(const char *path, size_t *size);
bool area512_ensure_directory(const char *path);
bool area512_ensure_parent_directories(char *resolved_path);

char *area512_read_next_manifest_entry(char **manifest_cursor);
void area512_join_manifest_entry_path(
  const char *directory_path,
  const char *entry_name,
  char *entry_path,
  int entry_path_size
);

void area512_usb_receive_files(void);
bool area512_usb_receive_owns_rxfifo(void);

void area512_restore_factory_boot_partition(void);
int area512_read_installed_app_name(char *name, size_t name_size);
int area512_install_app_image(
  const char *path,
  char *message,
  size_t message_size
);
int area512_launch_installed_app(char *message, size_t message_size);
int area512_uninstall_app(char *message, size_t message_size);

void area512_theme_load(void);
uint32_t area512_theme_background_color(void);
const char *area512_theme_background_image(void);
uint32_t area512_theme_text_color(void);
uint32_t area512_theme_emphasis_color(void);
uint32_t area512_theme_border_color(void);
uint32_t area512_theme_selected_color(void);
uint32_t area512_theme_box_color(void);
bool area512_theme_is_crt_enabled(void);
uint32_t area512_theme_blend_text_color_over_background(int text_color_percent);
uint32_t area512_theme_blend_selected_color_over_background(int selected_color_percent);
uint32_t area512_theme_blend_white_over_emphasis_color(int white_percent);
void area512_theme_pick_bitmap_colors(
  uint32_t *set_bit_color,
  uint32_t *clear_bit_color
);

void area512_default_ui_load(void);
int area512_default_ui_is_terminal(void);

// The app the filer runs as soon as the current one returns, as a path under
// AREA512_ROOT_PATH (an app directory, or a .rb/.mrb/.py/.mpy file). An empty
// path cancels. set returns -1 when the path is too long. take hands the path
// over once and clears it, so a failing app cannot start a loop.
int area512_set_next_app(const char *path);
int area512_take_next_app(char *path, size_t path_size);
void area512_cancel_next_app(void);

#ifdef AREA512_EXT_DISPLAY
#include "../captft/include/area512_hal.h"
#endif

#ifdef __cplusplus
}
#endif
