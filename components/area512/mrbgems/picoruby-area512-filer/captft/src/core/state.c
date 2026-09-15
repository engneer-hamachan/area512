#if defined(PICORB_VM_MRUBYC)

#include "core/filer.h"

#include <stddef.h>
#include <string.h>
#include <strings.h>

void
init_filer_state(Filer *filer) {
  filer->row = NULL;
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
