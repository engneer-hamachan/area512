#include "core/draw/draw.h"

// Two-row function bar at the bottom. key_x and label_x spread the chips over
// the bar width; they are fixed because the screen size is.
typedef struct {
  const char *key;
  const char *label;
  int key_x;
  int label_x;
} Chip;

static const Chip BAR1[] =
  {
    {"[ENT]", "open", 17, 52},
    {"[e]", "vim", 83, 106},
    {"[x]", "del", 130, 153},
    {"[N]", "new", 178, 201},
    {"[C]", "cp", 226, 249},
    {"[u]", "up", 267, 290}
  };

static const Chip BAR2[] =
  {
    {"[K]", "dir", 17, 41},
    {"[c]", "cmp", 66, 90},
    {"[a]", "all", 114, 138},
    {"[R]", "run", 163, 187},
    {"[m]", "mv", 212, 236},
    {"[t]", "term", 254, 278}
  };

#define BAR1_COUNT ((int)(sizeof(BAR1) / sizeof(BAR1[0])))
#define BAR2_COUNT ((int)(sizeof(BAR2) / sizeof(BAR2[0])))
#define BAR_TEXT_TOP ((BAR_HEIGHT - FILER_FONT_SIZE + FRAME_BOTTOM_HEIGHT) / 2)

static void
draw_divider(Filer *filer, int y) {
  area512_sprite_line(
    filer->screen,
    FRAME_INSET + 2,
    y,
    SCREEN_WIDTH - FRAME_INSET - 3,
    y,
    area512_theme_border_color()
  );
}

static void
draw_chips(Filer *filer, const Chip *chips, int count, int y, int divider) {
  if (divider)
    draw_divider(filer, y);

  int text_y = y + BAR_TEXT_TOP;

  for (int i = 0; i < count; i++) {
    area512_sprite_text(
      filer->screen,
      chips[i].key_x,
      text_y,
      chips[i].key,
      area512_theme_emphasis_color()
    );

    area512_sprite_text(
      filer->screen,
      chips[i].label_x,
      text_y,
      chips[i].label,
      area512_theme_text_color()
    );
  }
}

void
draw_primary_action_bar(Filer *filer) {
  if (filer->message[0]) {
    draw_divider(filer, BAR1_Y);

    char fitted[LINE_MAX];

    fit_string(fitted, sizeof fitted, filer->message, CONTENT_COLUMNS);

    area512_sprite_text(
      filer->screen,
      CONTENT_X,
      BAR1_Y + BAR_TEXT_TOP,
      fitted,
      area512_theme_emphasis_color()
    );
  } else {
    draw_chips(filer, BAR1, BAR1_COUNT, BAR1_Y, 1);
  }
}

void
draw_secondary_action_bar(Filer *filer) {
  draw_chips(filer, BAR2, BAR2_COUNT, BAR2_Y, 0);
}
