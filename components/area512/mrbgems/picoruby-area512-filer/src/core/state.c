#if defined(PICORB_VM_MRUBYC)

#include "core/filer.h"
#include "area512_hal.h"

#include <stddef.h>

#define PANEL_WIDTH 90
#define PANEL_INSET_RIGHT 14
#define PANEL_GAP 6

void
init_filer_state(Filer *filer) {
  filer->row = NULL;
  filer->width = area512_gfx_width();
  filer->height = area512_gfx_height();
  filer->content_x = 5;
  filer->content_right = filer->width - 8;
  filer->columns = (filer->content_right - filer->content_x) / FILER_CHAR_WIDTH;

  if (filer->columns < 1)
    filer->columns = 1;

  filer->edge_columns = (filer->width - 6) / FILER_CHAR_WIDTH;
  filer->list_top = ROW_HEIGHT;

  filer->rows_visible =
    (filer->height - filer->list_top - ROW_HEIGHT * 2 - 3) / ROW_HEIGHT;

  if (filer->rows_visible < 1)
    filer->rows_visible = 1;

  filer->bar1_y = filer->list_top + filer->rows_visible * ROW_HEIGHT;
  filer->bar2_y = filer->bar1_y + ROW_HEIGHT;
  filer->close_y = filer->bar2_y + ROW_HEIGHT;
  filer->panel_right = filer->width - PANEL_INSET_RIGHT;
  filer->panel_x = filer->panel_right - PANEL_WIDTH;

  if (filer->panel_x < filer->content_x + 10)
    filer->panel_x = filer->content_x + 10;

  filer->list_columns_panel =
    (filer->panel_x - PANEL_GAP - filer->content_x) / FILER_CHAR_WIDTH;

  if (filer->list_columns_panel < 1)
    filer->list_columns_panel = 1;

  filer->current_directory[0] = '/';
  filer->current_directory[1] = 0;
}

void
clamp_index(Filer *filer) {
  if (filer->count <= 0) {
    filer->index = 0;
    return;
  }

  if (filer->index >= filer->count)
    filer->index = filer->count - 1;

  if (filer->index < 0)
    filer->index = 0;
}

void
move_cursor(Filer *filer, int delta) {
  filer->message[0] = 0;
  filer->index += delta;
  clamp_index(filer);
}

void
jump_to(Filer *filer, int offset) {
  filer->message[0] = 0;
  int index = filer->top + offset;

  if (index < filer->count)
    filer->index = index;
}

int
selected_editable(Filer *filer) {
  if (filer->count == 0)
    return 0;

  FileEntry *entry = &filer->entries[filer->index];

  if (entry->type == ENTRY_TYPE_APP)
    return entry->has_rb;

  return entry->type == ENTRY_TYPE_OTHER;
}

#endif
