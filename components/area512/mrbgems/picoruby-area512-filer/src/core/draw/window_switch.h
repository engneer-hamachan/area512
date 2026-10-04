#pragma once

#include "core/filer.h"

WindowRect compute_window_rect(const Filer *filer);
void draw_window_switch_shape(Filer *filer);
void mark_window_switch_rows_changed(const Filer *filer, uint8_t *rows);
void load_window_image_bitmap(Filer *filer);
void free_window_image_bitmap(Filer *filer);
void ensure_window_open_with_switch_on(Filer *filer);
void ensure_window_closed_with_switch_off(Filer *filer);
