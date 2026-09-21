#pragma once

void area512_sprite_set_title_font(void *p);
void area512_sprite_round_rect(
  void *p,
  int x,
  int y,
  int w,
  int h,
  int corner_radius,
  uint32_t color
);

#define AREA512_WINDOW_WIDTH 240
#define AREA512_WINDOW_HEIGHT 135

// Drawing coordinates and reported dimensions use this window. Zero width
// and height restore the whole display.
void area512_gfx_set_window(int x, int y, int width, int height);
void area512_gfx_show_bitmap(const void *bitmap, int line_delay_milliseconds);

// Pair each successful begin with end. The panel keeps its contents after end.
int area512_internal_display_begin(void);
void area512_internal_display_end(void);
int area512_internal_display_width(void);
int area512_internal_display_height(void);
void area512_internal_display_fill_rect(
  int x,
  int y,
  int width,
  int height,
  uint32_t color
);
void area512_internal_display_text(
  int x,
  int y,
  const char *text,
  int byte_length,
  int font_size,
  uint32_t foreground,
  uint32_t background
);
