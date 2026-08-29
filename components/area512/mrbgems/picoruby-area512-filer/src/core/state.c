#if defined(PICORB_VM_MRUBYC)

#include "area512_hal.h"
#include "core/filer.h"

#include <stddef.h>
#include <string.h>
#include <strings.h>

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

static int
compare_entry_order(const FileEntry *left, const FileEntry *right) {
  if (left->type != right->type)
    return left->type < right->type ? -1 : 1;

  return strcasecmp(left->name, right->name);
}

void
add_entry_in_order(
  FileEntry *entries,
  int *entry_count,
  int entry_capacity,
  const FileEntry *entry
) {

  int add_index = *entry_count;

  if (*entry_count >= entry_capacity)
    return;

  while (
    add_index > 0 &&
    compare_entry_order(&entries[add_index - 1], entry) > 0
  ) {
    entries[add_index] = entries[add_index - 1];
    add_index--;
  }

  entries[add_index] = *entry;

  (*entry_count)++;
}

static int
has_path_suffix(const char *path, const char *suffix) {
  size_t path_byte_length = strlen(path);
  size_t suffix_byte_length = strlen(suffix);

  if (path_byte_length < suffix_byte_length)
    return 0;

  return memcmp(
    path + path_byte_length - suffix_byte_length,
    suffix,
    suffix_byte_length
  ) == 0;
}

int
is_ruby_file_path(const char *file_path) {
  return has_path_suffix(file_path, ".rb") ||
    has_path_suffix(file_path, ".mrb");
}

int
is_python_file_path(const char *file_path) {
  return has_path_suffix(file_path, ".py") ||
    has_path_suffix(file_path, ".mpy");
}

int
is_markdown_file_path(const char *file_path) {
  return has_path_suffix(file_path, ".md");
}

int
is_dot_image_file_path(const char *file_path) {
  return has_path_suffix(file_path, ".a5d");
}

int
is_source_file_path(const char *file_path) {
  return has_path_suffix(file_path, ".rb") ||
    has_path_suffix(file_path, ".py");
}

int
is_editable_file_path(const char *file_path) {
  return !has_path_suffix(file_path, ".mrb") &&
    !has_path_suffix(file_path, ".mpy") &&
    !has_path_suffix(file_path, ".a5d");
}

FileEntry *
fetch_selected_entry(Filer *filer) {
  if (filer->count == 0)
    return NULL;

  return &filer->entries[filer->index];
}

int
is_selected_markdown_file(Filer *filer) {
  FileEntry *entry = fetch_selected_entry(filer);

  return entry && entry->type == ENTRY_TYPE_FILE &&
    is_markdown_file_path(entry->name);
}

int
is_selected_ruby_file(Filer *filer) {
  FileEntry *entry = fetch_selected_entry(filer);

  return entry && entry->type == ENTRY_TYPE_FILE &&
    is_ruby_file_path(entry->name);
}

int
is_selected_python_file(Filer *filer) {
  FileEntry *entry = fetch_selected_entry(filer);

  return entry && entry->type == ENTRY_TYPE_FILE &&
    is_python_file_path(entry->name);
}

int
is_selected_source_file(Filer *filer) {
  FileEntry *entry = fetch_selected_entry(filer);

  return entry && entry->type == ENTRY_TYPE_FILE &&
    is_source_file_path(entry->name);
}

int
is_selected_dot_image_file(Filer *filer) {
  FileEntry *entry = fetch_selected_entry(filer);

  return entry && entry->type == ENTRY_TYPE_FILE &&
    is_dot_image_file_path(entry->name);
}

int
is_selected_editable(Filer *filer) {
  FileEntry *entry = fetch_selected_entry(filer);

  return entry && entry->type == ENTRY_TYPE_FILE &&
    is_editable_file_path(entry->name);
}

#endif
