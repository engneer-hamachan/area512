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
  {{"K", "dir"}, {"c", "cmp"}, {"a", "all"}, {"R", "run"}, {"m", "mv"}};

#define BAR1_COUNT ((int)(sizeof(BAR1) / sizeof(BAR1[0])))
#define BAR2_COUNT ((int)(sizeof(BAR2) / sizeof(BAR2[0])))

static void
draw_divider(Filer *filer, int y) {
  area512_sprite_line(
    filer->screen,
    0,
    y,
    filer->width - 1,
    y,
    area512_theme_border_color()
  );
}

static void
draw_chips(Filer *filer, const Chip *chips, int count, int y, int divider) {
  if (divider)
    draw_divider(filer, y);

  int x = filer->content_x + 1;
  int i = 0;

  while (i < count) {
    char key[12];

    snprintf(key, sizeof key, "[%s]", chips[i].key);

    area512_sprite_text(
      filer->screen,
      x,
      y + 1,
      key,
      area512_theme_emphasis_color()
    );

    x +=
      (int)strlen(key) * FILER_CHAR_WIDTH + FILER_CHAR_WIDTH; // gap after [key]

    area512_sprite_text(
      filer->screen,
      x,
      y + 1,
      chips[i].label,
      area512_theme_text_color()
    );

    x += (int)strlen(chips[i].label) * FILER_CHAR_WIDTH +
         FILER_CHAR_WIDTH; // gap to next chip

    i++;
  }
}

// Top bar: shows a message or input state instead of chips when one is set.
void
draw_primary_action_bar(Filer *filer) {
  if (filer->message[0]) {
    draw_divider(filer, filer->bar1_y);

    char fitted[LINE_MAX];

    fit_string(fitted, sizeof fitted, filer->message, filer->columns);

    area512_sprite_text(
      filer->screen,
      filer->content_x,
      filer->bar1_y + 1,
      fitted,
      area512_theme_emphasis_color()
    );

  } else {
    draw_chips(filer, BAR1, BAR1_COUNT, filer->bar1_y, 1);
  }
}

void
draw_secondary_action_bar(Filer *filer) {
  draw_chips(filer, BAR2, BAR2_COUNT, filer->bar2_y, 0);
}
