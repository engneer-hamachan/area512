#pragma once

typedef struct {
  int left, top, width, height;
} WindowRect;

typedef struct {
  WindowRect lit_line_rect;
  int draws_lit_line;
  int has_body;
  WindowRect body_rect;
} WindowSwitchShape;
