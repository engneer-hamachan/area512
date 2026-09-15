#include "core/draw/draw.h"

#include <stdio.h>
#include <string.h>

// Scrollbar color for row; -1 if everything fits (no bar).
static int
scrollbar_color(Filer *filer, int row) {
  int total = filer->count;

  if (total <= filer->rows_visible)
    return -1;

  int visible = filer->rows_visible;
  int thumb_length = visible * visible / total;

  if (thumb_length < 1)
    thumb_length = 1;

  int max_top = total - visible;
  int max_bar_top = visible - thumb_length;

  int thumb_top =
    max_top > 0 ? (filer->top * max_bar_top + max_top / 2) / max_top : 0;

  if (thumb_top + thumb_length > visible)
    thumb_top = visible - thumb_length;

  if (thumb_top < 0)
    thumb_top = 0;

  return (row >= thumb_top && row < thumb_top + thumb_length)
           ? area512_theme_border_color()
           : area512_theme_text_color();
}

static void
format_file_label(FileEntry *entry, char *out, int out_size) {
  if (entry->type == ENTRY_TYPE_UP)
    snprintf(out, out_size, "..");

  else if (entry->type == ENTRY_TYPE_DIR)
    snprintf(out, out_size, "%s/", entry->name);

  else
    snprintf(out, out_size, "%s", entry->name);
}

void
draw_entry(Filer *filer, int y, int index, int row, const PanelInfo *info) {
  if (
    y + ROW_HEIGHT <= area512_screen_region_top(filer->screen) ||
    y >= area512_screen_region_bottom(filer->screen)
  )
    return;

  area512_sprite_fill(filer->row, area512_theme_background_color());

  if (filer->has_background_image)
    area512_screen_read_sprite(filer->screen, filer->row, 0, y);

  if (index < filer->count) {
    FileEntry *entry = &filer->entries[index];
    int selected = (index == filer->index);

    char label_text[NAME_MAX + 2];
    format_file_label(entry, label_text, sizeof label_text);

    char line[LINE_MAX];

    snprintf(
      line,
      sizeof line,
      "%c %s",
      selected ? '>' : ' ', label_text
    );

    int list_columns =
      panel_covers_row(filer, row, info)
        ? filer->list_columns_panel
        : filer->columns;

    char fitted[LINE_MAX];
    fit_string(fitted, sizeof fitted, line, list_columns);

    area512_sprite_text(
      filer->row,
      filer->content_x,
      0,
      fitted,
      selected ? area512_theme_selected_color() : area512_theme_text_color()
    );
  }

  int scrollbar = scrollbar_color(filer, row);

  if (scrollbar >= 0) {
    area512_sprite_fill_rect(
      filer->row,
      filer->width - 6,
      1,
      2,
      ROW_HEIGHT - 2,
      (uint32_t)scrollbar
    );
  }

  draw_panel_row(filer, row, info);
  area512_screen_draw_sprite(filer->screen, filer->row, 0, y);
}

void
adjust_scroll(Filer *filer) {
  if (filer->index < filer->top) {
    filer->top = filer->index;
  } else if (filer->index >= filer->top + filer->rows_visible) {
    filer->top = filer->index - filer->rows_visible + 1;
  }

  if (filer->top < 0)
    filer->top = 0;
}
