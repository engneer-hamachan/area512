#if defined(PICORB_VM_MRUBYC)

#include "core/dot_editor.h"

#include <string.h>

static int
fetch_color_index_for_key(int key_byte) {
  if (key_byte >= '0' && key_byte <= '9')
    return key_byte - '0';

  if (key_byte >= 'a' && key_byte <= 'f')
    return key_byte - 'a' + 10;

  return -1;
}

static void
select_color(DotEditor *editor, int color_index) {
  if (color_index < 0 || color_index >= editor->image->palette_color_count)
    return;

  editor->selected_color_index = color_index;
  editor->selection = DOT_EDITOR_SELECTION_COLOR;
}

static void
select_hitbox_add(DotEditor *editor) {
  editor->selection = DOT_EDITOR_SELECTION_HITBOX_ADD;
}

static void
select_hitbox_delete(DotEditor *editor) {
  editor->selection = DOT_EDITOR_SELECTION_HITBOX_DELETE;
}

static void
paint_selection_at_cursor(DotEditor *editor) {
  switch (editor->selection) {
  case DOT_EDITOR_SELECTION_COLOR:
    set_dot_image_color_index(
      editor->image,
      editor->cursor_dot_x,
      editor->cursor_dot_y,
      editor->selected_color_index
    );

    return;

  case DOT_EDITOR_SELECTION_HITBOX_ADD:
    set_dot_image_hitbox(
      editor->image,
      editor->cursor_dot_x,
      editor->cursor_dot_y,
      1
    );

    return;

  case DOT_EDITOR_SELECTION_HITBOX_DELETE:
    set_dot_image_hitbox(
      editor->image,
      editor->cursor_dot_x,
      editor->cursor_dot_y,
      0
    );

    return;
  }
}

void
initialize_dot_editor(
  DotEditor *editor,
  DotImage *image,
  const char *path,
  int path_byte_length,
  void *screen_sprite
) {

  int byte_length = path_byte_length;

  if (byte_length > (int)sizeof(editor->path) - 1)
    byte_length = (int)sizeof(editor->path) - 1;

  if (byte_length < 0)
    byte_length = 0;

  memcpy(editor->path, path, (size_t)byte_length);
  editor->path[byte_length] = '\0';

  editor->image = image;
  editor->screen_sprite = screen_sprite;
  editor->path_byte_length = byte_length;
  editor->cursor_dot_x = 0;
  editor->cursor_dot_y = 0;
  editor->selected_color_index = 1;
  editor->selection = DOT_EDITOR_SELECTION_COLOR;
  editor->is_pen_down = 0;
  editor->is_grid_visible = 1;
}

DotEditorStatus
handle_dot_editor_key(DotEditor *editor, int key_byte) {
  int color_index = fetch_color_index_for_key(key_byte);

  if (color_index >= 0) {
    select_color(editor, color_index);
    return DOT_EDITOR_STATUS_CONTINUE;
  }

  switch (key_byte) {
  case 'h':
    move_dot_editor_cursor_to(
      editor,
      editor->cursor_dot_x - 1,
      editor->cursor_dot_y
    );

    break;

  case 'l':
    move_dot_editor_cursor_to(
      editor,
      editor->cursor_dot_x + 1,
      editor->cursor_dot_y
    );

    break;

  case 'k':
    move_dot_editor_cursor_to(
      editor,
      editor->cursor_dot_x,
      editor->cursor_dot_y - 1
    );

    break;

  case 'j':
    move_dot_editor_cursor_to(
      editor,
      editor->cursor_dot_x,
      editor->cursor_dot_y + 1
    );

    break;

  case ' ':
    paint_selection_at_cursor(editor);
    return DOT_EDITOR_STATUS_CONTINUE;

  case 'p':
    toggle_dot_editor_pen(editor);

    if (editor->is_pen_down)
      paint_selection_at_cursor(editor);

    return DOT_EDITOR_STATUS_CONTINUE;

  case 'g':
    toggle_dot_editor_grid(editor);
    return DOT_EDITOR_STATUS_CONTINUE;

  case 'x':
    select_hitbox_add(editor);
    return DOT_EDITOR_STATUS_CONTINUE;

  case 'z':
    select_hitbox_delete(editor);
    return DOT_EDITOR_STATUS_CONTINUE;

  case 's':
    return DOT_EDITOR_STATUS_SAVE;

  case 'q':
    return DOT_EDITOR_STATUS_QUIT;

  default:
    return DOT_EDITOR_STATUS_CONTINUE;
  }

  if (editor->is_pen_down)
    paint_selection_at_cursor(editor);

  return DOT_EDITOR_STATUS_CONTINUE;
}

void
move_dot_editor_cursor_to(DotEditor *editor, int dot_x, int dot_y) {
  if (dot_x < 0)
    dot_x = 0;

  if (dot_x >= editor->image->width)
    dot_x = editor->image->width - 1;

  if (dot_y < 0)
    dot_y = 0;

  if (dot_y >= editor->image->height)
    dot_y = editor->image->height - 1;

  editor->cursor_dot_x = dot_x;
  editor->cursor_dot_y = dot_y;
}

void
toggle_dot_editor_pen(DotEditor *editor) {
  editor->is_pen_down = !editor->is_pen_down;
}

void
toggle_dot_editor_grid(DotEditor *editor) {
  editor->is_grid_visible = !editor->is_grid_visible;
}

#endif
