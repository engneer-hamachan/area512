#include "core/draw/draw.h"

#include <string.h>

static void
find_changed_entry_rows(Filer *filer, uint8_t *rows) {
  FilerDrawState *drawn = &filer->drawn;
  PanelInfo *panel_info = &filer->panel_info;
  int panel_changed = 0;

  for (int metric = 0; metric < panel_info->count; metric++) {
    if (panel_info->percent[metric] != drawn->panel_percent[metric])
      panel_changed = 1;
  }

  for (int row = 0; row < ROWS_VISIBLE; row++) {
    int index = filer->top + row;
    int old_index = drawn->top + row;
    int present = index < filer->count;
    int old_present = old_index < drawn->count;
    int selected = present && index == filer->index;

    int changed =
      selected ||
      filer->top != drawn->top ||
      filer->count != drawn->count ||
      present != old_present ||
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
}

void
find_changed_rows(Filer *filer, uint8_t *rows) {
  FilerDrawState *drawn = &filer->drawn;

  memset(rows, 0, SCREEN_HEIGHT);

  if (
    filer->full_redraw ||
    !drawn->valid ||
    !area512_theme_background_image()[0]
  ) {

    memset(rows, 1, SCREEN_HEIGHT);
    return;
  }

  // header -3/4-
  if (
    filer->index != drawn->index ||
    filer->count != drawn->count ||
    strcmp(filer->current_directory, drawn->current_directory) != 0
  )
    memset(rows, 1, HEADER_HEIGHT);

  if (area512_is_filer_window_close_animation_running())
    memset(
      rows + filer->closing_window_top - 1,
      1,
      AREA512_WINDOW_HEIGHT + 2
    );

  if (
    filer->is_window_open ||
    area512_is_filer_window_open_animation_running() ||
    area512_is_filer_window_close_animation_running()
  )
    memset(rows + find_window_top(filer) - 1, 1, AREA512_WINDOW_HEIGHT + 2);
  else
    find_changed_entry_rows(filer, rows);

  if (strcmp(filer->message, drawn->message) != 0) {
    memset(rows + BAR_Y, 1, BAR_TO_FRAME_BOTTOM_ROW_COUNT);
    memset(rows + SOFT_KEYBOARD_MESSAGE_Y, 1, SOFT_KEYBOARD_MESSAGE_HEIGHT);
  }

  if (filer->pressed_chip_index != drawn->pressed_chip_index)
    memset(rows + BAR_Y, 1, BAR_TO_FRAME_BOTTOM_ROW_COUNT);
}

void
save_draw_state(Filer *filer) {
  FilerDrawState *drawn = &filer->drawn;

  drawn->index = filer->index;
  drawn->top = filer->top;
  drawn->count = filer->count;

  memcpy(
    drawn->current_directory,
    filer->current_directory,
    CURRENT_DIRECTORY_MAX
  );

  memcpy(drawn->message, filer->message, MESSAGE_MAX);
  drawn->pressed_chip_index = filer->pressed_chip_index;

  for (int row = 0; row < ROWS_VISIBLE; row++) {
    int index = filer->top + row;

    if (index < filer->count)
      drawn->entries[row] = filer->entries[index];
  }

  for (int metric = 0; metric < filer->panel_info.count; metric++)
    drawn->panel_percent[metric] = filer->panel_info.percent[metric];
}
