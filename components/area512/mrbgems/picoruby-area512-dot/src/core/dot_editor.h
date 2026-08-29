#pragma once

#include "area512_hal.h"
#include "core/dot_image.h"

#define DOT_EDITOR_WIDTH 32
#define DOT_EDITOR_HEIGHT 32
#define DOT_EDITOR_FONT_SIZE 12

typedef enum {
  DOT_EDITOR_STATUS_CONTINUE,
  DOT_EDITOR_STATUS_SAVE,
  DOT_EDITOR_STATUS_QUIT
} DotEditorStatus;

typedef enum {
  DOT_EDITOR_SELECTION_COLOR,
  DOT_EDITOR_SELECTION_HITBOX_ADD,
  DOT_EDITOR_SELECTION_HITBOX_DELETE
} DotEditorSelection;

typedef struct {
  DotImage *image;
  void *screen_sprite;
  char path[AREA512_PATH_MAX];
  int path_byte_length;
  int cursor_dot_x;
  int cursor_dot_y;
  int selected_color_index;
  DotEditorSelection selection;
  int is_pen_down;
  int is_grid_visible;
} DotEditor;

void initialize_dot_editor(
  DotEditor *editor,
  DotImage *image,
  const char *path,
  int path_byte_length,
  void *screen_sprite
);

DotEditorStatus handle_dot_editor_key(DotEditor *editor, int key_byte);

void move_dot_editor_cursor_to(DotEditor *editor, int dot_x, int dot_y);

void toggle_dot_editor_pen(DotEditor *editor);
void toggle_dot_editor_grid(DotEditor *editor);
