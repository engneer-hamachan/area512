#include "core/draw/draw.h"

typedef struct {
  const char *number;
  const char *label;
  int key_code;
} Chip;

static const Chip CHIPS[] =
  {
    {"01", "TRM", KEY_TERMINAL},
    {"02", "RUN", KEY_RUN_DIR},
    {"03", "EDT", KEY_EDIT}
  };

#define CHIP_COUNT ((int)(sizeof(CHIPS) / sizeof(CHIPS[0])))
#define CELL_LEFT (FRAME_INSET + 2)
#define CELL_WIDTH ((SCREEN_WIDTH - CELL_LEFT * 2) / CHIP_COUNT)
#define CELL_TOP (BAR_Y + 1)
#define CELL_BOTTOM (CLOSE_Y + FRAME_BOTTOM_HEIGHT - 3)
#define CHIP_CENTER_Y ((CELL_TOP + CELL_BOTTOM) / 2 + 1)
#define CHIP_TEXT_LINE_GAP 2
#define BAR_MESSAGE_TEXT_OFFSET \
  ((BAR_HEIGHT - FILER_FONT_SIZE + FRAME_BOTTOM_HEIGHT) / 2)
#define SOFT_KEYBOARD_MESSAGE_TEXT_OFFSET \
  ((SOFT_KEYBOARD_MESSAGE_HEIGHT - FILER_FONT_SIZE) / 2)
#define BAR_INNER_LEFT (FRAME_INSET + 2)
#define BAR_INNER_BOTTOM (CLOSE_Y + FRAME_BOTTOM_HEIGHT - 2)
#define BAR_INNER_CORNER_RADIUS (FRAME_RADIUS - 2)

static void
draw_divider(Filer *filer, int screen_y) {
  area512_sprite_line(
    filer->screen,
    FRAME_INSET + 2,
    screen_y,
    SCREEN_WIDTH - FRAME_INSET - 3,
    screen_y,
    area512_theme_border_color()
  );
}

static int
compute_chip_text_left(Filer *filer, const Chip *chip, int center_x) {
  int number_width = area512_sprite_text_width(filer->screen, chip->number);
  int label_width = area512_sprite_text_width(filer->screen, chip->label);
  int chip_text_width = number_width > label_width ? number_width : label_width;

  return center_x - chip_text_width / 2;
}

static void
fill_pressed_chip(Filer *filer, int cell_left) {
  for (int screen_y = CELL_TOP; screen_y <= CELL_BOTTOM; screen_y++) {
    int inset =
      compute_corner_inset(
        screen_y - (BAR_INNER_BOTTOM - BAR_INNER_CORNER_RADIUS - 1),
        BAR_INNER_CORNER_RADIUS
      );

    int fill_screen_left = BAR_INNER_LEFT + inset;
    int fill_screen_right = SCREEN_WIDTH - (BAR_INNER_LEFT + inset);

    if (cell_left > fill_screen_left)
      fill_screen_left = cell_left;

    if (cell_left + CELL_WIDTH < fill_screen_right)
      fill_screen_right = cell_left + CELL_WIDTH;

    if (fill_screen_left >= fill_screen_right)
      continue;

    area512_sprite_fill_rect(
      filer->screen,
      fill_screen_left,
      screen_y,
      fill_screen_right - fill_screen_left,
      1,
      area512_theme_selected_color()
    );
  }
}

static void
draw_chips(Filer *filer) {
  int font_height = area512_sprite_font_height(FILER_FONT_SIZE);
  int number_top = CHIP_CENTER_Y - (font_height * 2 + CHIP_TEXT_LINE_GAP) / 2;
  int label_top = number_top + font_height + CHIP_TEXT_LINE_GAP;

  draw_divider(filer, BAR_Y);

  for (int i = 0; i < CHIP_COUNT; i++) {
    int cell_left = CELL_LEFT + i * CELL_WIDTH;
    int center_x = cell_left + CELL_WIDTH / 2;
    int chip_text_left = compute_chip_text_left(filer, &CHIPS[i], center_x);

    if (i == filer->pressed_chip_index)
      fill_pressed_chip(filer, cell_left);

    if (i > 0)
      area512_sprite_line(
        filer->screen,
        cell_left,
        CELL_TOP,
        cell_left,
        CELL_BOTTOM,
        area512_theme_border_color()
      );

    area512_sprite_text(
      filer->screen,
      chip_text_left,
      number_top,
      CHIPS[i].number,
      area512_theme_emphasis_color()
    );

    area512_sprite_text(
      filer->screen,
      chip_text_left,
      label_top,
      CHIPS[i].label,
      area512_theme_border_color()
    );
  }
}

static void
draw_message(Filer *filer, int screen_y) {
  char fitted[LINE_MAX];

  fit_string(fitted, sizeof fitted, filer->message, CONTENT_COLUMNS);

  area512_sprite_text(
    filer->screen,
    CONTENT_X,
    screen_y,
    fitted,
    area512_theme_emphasis_color()
  );
}

void
draw_action_bar(Filer *filer) {
  if (filer->message[0] && !area512_is_soft_keyboard_open()) {
    draw_divider(filer, BAR_Y);
    draw_message(filer, BAR_Y + BAR_MESSAGE_TEXT_OFFSET);
  } else {
    draw_chips(filer);
  }
}

void
draw_message_above_soft_keyboard(Filer *filer) {
  if (!filer->message[0] || !area512_is_soft_keyboard_open())
    return;

  area512_sprite_fill_rect(
    filer->screen,
    FRAME_INSET + 2,
    SOFT_KEYBOARD_MESSAGE_Y,
    SCREEN_WIDTH - (FRAME_INSET + 2) * 2,
    SOFT_KEYBOARD_MESSAGE_HEIGHT,
    area512_theme_background_color()
  );

  draw_divider(filer, SOFT_KEYBOARD_MESSAGE_Y);

  draw_message(
    filer,
    SOFT_KEYBOARD_MESSAGE_Y + SOFT_KEYBOARD_MESSAGE_TEXT_OFFSET
  );
}

static int
find_chip_index_at(int touch_screen_x) {
  if (touch_screen_x < CELL_LEFT)
    return -1;

  int chip_index = (touch_screen_x - CELL_LEFT) / CELL_WIDTH;

  return chip_index < CHIP_COUNT ? chip_index : -1;
}

int
is_action_bar_at(int touch_screen_y) {
  return touch_screen_y >= CELL_TOP && touch_screen_y <= CELL_BOTTOM;
}

int
find_action_bar_key_at(int touch_screen_x, int touch_screen_y) {
  if (!is_action_bar_at(touch_screen_y))
    return KEY_NONE;

  int chip_index = find_chip_index_at(touch_screen_x);

  return chip_index >= 0 ? CHIPS[chip_index].key_code : KEY_NONE;
}

int
find_pressed_chip_index(Filer *filer) {
  if (filer->message[0] && !area512_is_soft_keyboard_open())
    return -1;

  int touch_screen_x;
  int touch_screen_y;

  if (
    !area512_read_touch_start_point(&touch_screen_x, &touch_screen_y) ||
    !is_action_bar_at(touch_screen_y)
  )
    return -1;

  return find_chip_index_at(touch_screen_x);
}
