#if defined(PICORB_VM_MRUBYC)

#include "area512_hal.h"
#include "core/dot_draw.h"
#include "core/dot_image.h"

#include <stdio.h>

#define CANVAS_LEFT 4
#define CANVAS_TOP 3
#define CANVAS_CELL_PIXEL_SIZE 4
#define CANVAS_GRID_LINE_PIXEL_SIZE 1

#define PALETTE_LEFT 140
#define PALETTE_TOP 4
#define PALETTE_TILE_PIXEL_SIZE 14
#define PALETTE_TILE_PIXEL_GAP 1
#define PALETTE_COLUMN_COUNT 4
#define PALETTE_TILE_FONT_SIZE 10
#define PALETTE_TILE_FONT_PIXEL_HEIGHT 10

#define PALETTE_HITBOX_LEFT \
  (PALETTE_LEFT + \
   PALETTE_COLUMN_COUNT * (PALETTE_TILE_PIXEL_SIZE + PALETTE_TILE_PIXEL_GAP))

#define PALETTE_HITBOX_TOP PALETTE_TOP

#define PALETTE_HITBOX_DELETE_TOP \
  (PALETTE_HITBOX_TOP + PALETTE_TILE_PIXEL_SIZE + PALETTE_TILE_PIXEL_GAP)

#define CANVAS_HITBOX_PIXEL_SIZE 3

#define PREVIEW_LEFT 140
#define PREVIEW_TOP 70

#define STATUS_LEFT 140
#define STATUS_TOP 110
#define STATUS_TEXT_MAX 24
#define STATUS_COLOR_TEXT_MAX 4

#define BACKGROUND_RGB888 0x111111
#define TEXT_RGB888 0xDDDDDD
#define GRID_RGB888 0x222222
#define CURSOR_RGB888 0xFFFFFF
#define TRANSPARENT_CELL_RGB888 0x303840
#define PALETTE_FRAME_RGB888 0x695AFF
#define HITBOX_RGB888 0x00FF87
#define DARK_TEXT_RGB888 0x000000
#define LIGHT_TEXT_RGB888 0xFFFFFF

static void
draw_canvas_cell(const DotEditor *editor, int dot_x, int dot_y) {
  int left = CANVAS_LEFT + dot_x * CANVAS_CELL_PIXEL_SIZE;
  int top = CANVAS_TOP + dot_y * CANVAS_CELL_PIXEL_SIZE;
  int color_index = fetch_dot_image_color_index(editor->image, dot_x, dot_y);

  uint32_t rgb888 =
    color_index == DOT_IMAGE_TRANSPARENT_COLOR_INDEX
      ? TRANSPARENT_CELL_RGB888
      : convert_rgb565_to_rgb888(editor->image->palette_rgb565[color_index]);

  area512_sprite_fill_rect(
    editor->screen_sprite,
    left,
    top,
    CANVAS_CELL_PIXEL_SIZE,
    CANVAS_CELL_PIXEL_SIZE,
    rgb888
  );

  if (editor->is_grid_visible) {
    area512_sprite_line(
      editor->screen_sprite,
      left,
      top,
      left + CANVAS_CELL_PIXEL_SIZE - 1,
      top,
      GRID_RGB888
    );

    area512_sprite_line(
      editor->screen_sprite,
      left,
      top,
      left,
      top + CANVAS_CELL_PIXEL_SIZE - 1,
      GRID_RGB888
    );
  }

  if (
    (
      editor->selection == DOT_EDITOR_SELECTION_HITBOX_ADD ||
      editor->selection == DOT_EDITOR_SELECTION_HITBOX_DELETE
    ) &&
    has_dot_image_hitbox(editor->image, dot_x, dot_y)
  ) {

    int hitbox_pixel_left =
      left + CANVAS_GRID_LINE_PIXEL_SIZE +
      (CANVAS_CELL_PIXEL_SIZE - CANVAS_GRID_LINE_PIXEL_SIZE -
       CANVAS_HITBOX_PIXEL_SIZE) / 2;

    int hitbox_pixel_top =
      top + CANVAS_GRID_LINE_PIXEL_SIZE +
      (CANVAS_CELL_PIXEL_SIZE - CANVAS_GRID_LINE_PIXEL_SIZE -
       CANVAS_HITBOX_PIXEL_SIZE) / 2;

    area512_sprite_fill_rect(
      editor->screen_sprite,
      hitbox_pixel_left,
      hitbox_pixel_top,
      CANVAS_HITBOX_PIXEL_SIZE,
      CANVAS_HITBOX_PIXEL_SIZE,
      HITBOX_RGB888
    );
  }

  if (dot_x == editor->cursor_dot_x && dot_y == editor->cursor_dot_y) {
    area512_sprite_rect(
      editor->screen_sprite,
      left,
      top,
      CANVAS_CELL_PIXEL_SIZE,
      CANVAS_CELL_PIXEL_SIZE,
      CURSOR_RGB888
    );
  }
}

static void
draw_canvas(const DotEditor *editor) {
  for (int dot_y = 0; dot_y < editor->image->height; dot_y++) {
    for (int dot_x = 0; dot_x < editor->image->width; dot_x++)
      draw_canvas_cell(editor, dot_x, dot_y);
  }
}

static char
fetch_palette_tile_character(int color_index) {
  if (color_index < 10)
    return (char)('0' + color_index);

  return (char)('a' + color_index - 10);
}

static int
is_light_rgb888(uint32_t rgb888) {
  uint32_t red = (rgb888 >> 16) & 0xFF;
  uint32_t green = (rgb888 >> 8) & 0xFF;
  uint32_t blue = rgb888 & 0xFF;

  return (299 * red + 587 * green + 114 * blue) / 1000 > 127;
}

static void
draw_palette_tile(
  const DotEditor *editor,
  int left,
  int top,
  uint32_t rgb888,
  const char *tile_text,
  int is_selected
) {

  area512_sprite_fill_rect(
    editor->screen_sprite,
    left,
    top,
    PALETTE_TILE_PIXEL_SIZE,
    PALETTE_TILE_PIXEL_SIZE,
    rgb888
  );

  int tile_text_width =
    area512_sprite_text_width(editor->screen_sprite, tile_text);

  area512_sprite_text(
    editor->screen_sprite,
    left + (PALETTE_TILE_PIXEL_SIZE - tile_text_width) / 2,
    top + (PALETTE_TILE_PIXEL_SIZE - PALETTE_TILE_FONT_PIXEL_HEIGHT) / 2,
    tile_text,
    is_light_rgb888(rgb888) ? DARK_TEXT_RGB888 : LIGHT_TEXT_RGB888
  );

  if (!is_selected)
    return;

  area512_sprite_rect(
    editor->screen_sprite,
    left,
    top,
    PALETTE_TILE_PIXEL_SIZE,
    PALETTE_TILE_PIXEL_SIZE,
    PALETTE_FRAME_RGB888
  );
}

static void
draw_palette(const DotEditor *editor) {
  area512_sprite_set_font_size(editor->screen_sprite, PALETTE_TILE_FONT_SIZE);

  for (
    int color_index = 0;
    color_index < editor->image->palette_color_count;
    color_index++
  ) {

    int column_index = color_index % PALETTE_COLUMN_COUNT;
    int row_index = color_index / PALETTE_COLUMN_COUNT;

    int left =
      PALETTE_LEFT +
      column_index * (PALETTE_TILE_PIXEL_SIZE + PALETTE_TILE_PIXEL_GAP);

    int top =
      PALETTE_TOP +
      row_index * (PALETTE_TILE_PIXEL_SIZE + PALETTE_TILE_PIXEL_GAP);

    uint32_t rgb888 =
      color_index == DOT_IMAGE_TRANSPARENT_COLOR_INDEX
        ? TRANSPARENT_CELL_RGB888
        : convert_rgb565_to_rgb888(editor->image->palette_rgb565[color_index]);

    char tile_text[2] = {
      fetch_palette_tile_character(color_index), '\0'
    };

    draw_palette_tile(
      editor,
      left,
      top,
      rgb888,
      tile_text,
      editor->selection == DOT_EDITOR_SELECTION_COLOR &&
        color_index == editor->selected_color_index
    );
  }

  draw_palette_tile(
    editor,
    PALETTE_HITBOX_LEFT,
    PALETTE_HITBOX_TOP,
    HITBOX_RGB888,
    "x",
    editor->selection == DOT_EDITOR_SELECTION_HITBOX_ADD
  );

  draw_palette_tile(
    editor,
    PALETTE_HITBOX_LEFT,
    PALETTE_HITBOX_DELETE_TOP,
    TRANSPARENT_CELL_RGB888,
    "z",
    editor->selection == DOT_EDITOR_SELECTION_HITBOX_DELETE
  );

  area512_sprite_set_font_size(editor->screen_sprite, DOT_EDITOR_FONT_SIZE);
}

static void
draw_preview(const DotEditor *editor) {
  push_dot_image_to_sprite(
    editor->image,
    editor->screen_sprite,
    PREVIEW_LEFT,
    PREVIEW_TOP
  );
}

static void
draw_status(const DotEditor *editor) {
  char status_text[STATUS_TEXT_MAX];
  char selected_color_text[STATUS_COLOR_TEXT_MAX];
  const char *selection_text = "hit";

  if (editor->selection == DOT_EDITOR_SELECTION_COLOR) {
    snprintf(
      selected_color_text,
      sizeof selected_color_text,
      "c%d",
      editor->selected_color_index
    );

    selection_text = selected_color_text;
  }

  if (editor->selection == DOT_EDITOR_SELECTION_HITBOX_DELETE)
    selection_text = "delhit";

  snprintf(
    status_text,
    sizeof status_text,
    "%s %d,%d %s",
    editor->is_pen_down ? "pen" : "move",
    editor->cursor_dot_x,
    editor->cursor_dot_y,
    selection_text
  );

  area512_sprite_text(
    editor->screen_sprite,
    STATUS_LEFT,
    STATUS_TOP,
    status_text,
    TEXT_RGB888
  );
}

void
draw_dot_editor_screen(const DotEditor *editor) {
  area512_sprite_fill(editor->screen_sprite, BACKGROUND_RGB888);

  draw_canvas(editor);
  draw_palette(editor);
  draw_preview(editor);
  draw_status(editor);

  area512_sprite_push(editor->screen_sprite, 0, 0);
}

#endif
