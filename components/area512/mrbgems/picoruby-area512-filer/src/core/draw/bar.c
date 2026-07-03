#include "core/draw/draw.h"

#include <stdio.h>
#include <string.h>

// Two-row function bar at the bottom.
typedef struct {
  const char *key;
  const char *label;
} Chip;

static const Chip BAR1[] =
  {{"ENT", "open"}, {"e", "vim"}, {"x", "del"}, {"N", "new"}};
static const Chip BAR2[] =
  {{"K", "dir"}, {"c", "compile"}, {"a", "all"}, {"R", "run"}};

#define BAR1_COUNT ((int)(sizeof(BAR1) / sizeof(BAR1[0])))
#define BAR2_COUNT ((int)(sizeof(BAR2) / sizeof(BAR2[0])))

static void
draw_chips(Filer *filer, const Chip *chips, int count, int y, int divider) {
  area512_sprite_fill(filer->row, COLOR_BACKGROUND);
  draw_walls(filer, 0);

  if (divider)
    area512_sprite_line(filer->row, 0, 0, filer->width - 1, 0, COLOR_GREEN);

  int x = filer->content_x + 1;
  int i = 0;

  while (i < count) {
    char key[12];

    snprintf(key, sizeof key, "[%s]", chips[i].key);
    area512_sprite_text(filer->row, x, 1, key, COLOR_GREEN);

    x += (int)strlen(key) * FILER_CHAR_WIDTH + FILER_CHAR_WIDTH; // gap after [key]

    area512_sprite_text(filer->row, x, 1, chips[i].label, COLOR_DIM);

    x += (int)strlen(chips[i].label) * FILER_CHAR_WIDTH + FILER_CHAR_WIDTH; // gap to next chip

    i++;
  }

  area512_sprite_push(filer->row, 0, y);
}

// Top bar: shows a message or input state instead of chips when one is set.
void
draw_primary_action_bar(Filer *filer) {
  if (filer->message[0]) {
    area512_sprite_fill(filer->row, COLOR_BACKGROUND);
    draw_walls(filer, 0);
    area512_sprite_line(filer->row, 0, 0, filer->width - 1, 0, COLOR_GREEN);

    char fitted[LINE_MAX];

    fit_string(fitted, sizeof fitted, filer->message, filer->columns);
    area512_sprite_text(filer->row, filer->content_x, 1, fitted, COLOR_AMBER);
    area512_sprite_push(filer->row, 0, filer->bar1_y);

  } else {
    draw_chips(filer, BAR1, BAR1_COUNT, filer->bar1_y, 1);
  }
}

void
draw_secondary_action_bar(Filer *filer) {
  draw_chips(filer, BAR2, BAR2_COUNT, filer->bar2_y, 0);
}
