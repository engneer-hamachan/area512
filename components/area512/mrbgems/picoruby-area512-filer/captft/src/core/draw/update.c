#include "core/draw/draw.h"

#include <string.h>

void
find_changed_rows(Filer *filer, const PanelInfo *info, uint8_t *rows) {
  FilerDrawState *drawn = &filer->drawn;

  memset(rows, 0, SCREEN_HEIGHT);

  if (
    filer->full_redraw || !drawn->valid || filer->draws_window_frame ||
    drawn->draws_window_frame || !area512_theme_background_image()[0]
  ) {
    memset(rows, 1, SCREEN_HEIGHT);
    return;
  }

  if (
    filer->index != drawn->index || filer->count != drawn->count ||
    strcmp(filer->current_directory, drawn->current_directory) != 0
  )
    memset(rows, 1, HEADER_HEIGHT);

  int panel_changed = 0;

  for (int metric = 0; metric < info->count; metric++) {
    if (info->percent[metric] != drawn->panel_percent[metric])
      panel_changed = 1;
  }

  for (int row = 0; row < ROWS_VISIBLE; row++) {
    int index = filer->top + row;
    int old_index = drawn->top + row;
    int present = index < filer->count;
    int old_present = old_index < drawn->count;

    int changed =
      filer->top != drawn->top || filer->count != drawn->count ||
      present != old_present ||
      (present && index == filer->index) !=
        (old_present && old_index == drawn->index);

    if (present && old_present) {
      FileEntry *entry = &filer->entries[index];
      FileEntry *old_entry = &drawn->entries[row];

      if (
        entry->type != old_entry->type ||
        strcmp(entry->name, old_entry->name) != 0
      )
        changed = 1;
    }

    if (panel_changed && panel_covers_row(row))
      changed = 1;

    if (changed)
      memset(rows + HEADER_HEIGHT + row * ROW_HEIGHT, 1, ROW_HEIGHT);
  }

  if (strcmp(filer->message, drawn->message) != 0)
    memset(rows + BAR1_Y, 1, BAR_HEIGHT);
}

void
save_draw_state(Filer *filer, const PanelInfo *info) {
  FilerDrawState *drawn = &filer->drawn;

  drawn->index = filer->index;
  drawn->top = filer->top;
  drawn->count = filer->count;
  drawn->draws_window_frame = filer->draws_window_frame;
  memcpy(
    drawn->current_directory,
    filer->current_directory,
    CURRENT_DIRECTORY_MAX
  );
  memcpy(drawn->message, filer->message, MESSAGE_MAX);

  for (int row = 0; row < ROWS_VISIBLE; row++) {
    int index = filer->top + row;

    if (index < filer->count)
      drawn->entries[row] = filer->entries[index];
  }

  for (int metric = 0; metric < info->count; metric++)
    drawn->panel_percent[metric] = info->percent[metric];
}
