#if defined(PICORB_VM_MRUBYC)

#include "area512_hal.h"
#include "core/terminal/terminal.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ENTRY_LAYOUT_COLUMN_CAPACITY 13

typedef struct {
  const FileEntry *entries;
  int entry_count;
} EntryList;

typedef struct {
  int column_byte_counts[ENTRY_LAYOUT_COLUMN_CAPACITY];
  int column_count;
  int row_count;
} EntryLayout;

static int
count_entry_label_bytes(const FileEntry *entry) {
  int entry_label_byte_count = (int)strlen(entry->name);

  if (entry->type == ENTRY_TYPE_DIR)
    entry_label_byte_count++;

  return entry_label_byte_count;
}

static void
set_layout_column_byte_counts(
  const EntryList *entry_list,
  EntryLayout *layout
) {

  int column_index = 0;
  int entry_index;
  int entry_label_byte_count;

  while (column_index < layout->column_count) {
    layout->column_byte_counts[column_index] = 0;
    entry_index = column_index;

    while (entry_index < entry_list->entry_count) {
      entry_label_byte_count =
        count_entry_label_bytes(&entry_list->entries[entry_index]);

      if (
        entry_label_byte_count >
        layout->column_byte_counts[column_index]
      )
        layout->column_byte_counts[column_index] = entry_label_byte_count;

      entry_index += layout->column_count;
    }

    column_index++;
  }
}

static int
count_layout_row_bytes(const EntryLayout *layout) {
  int row_byte_count = 0;
  int column_index = 0;

  while (column_index < layout->column_count) {
    row_byte_count += layout->column_byte_counts[column_index];

    column_index++;
  }

  return row_byte_count + 2 * (layout->column_count - 1);
}

static void
build_entry_layout(
  const Filer *filer,
  const EntryList *entry_list,
  EntryLayout *layout
) {

  int column_count = filer->terminal->line_byte_capacity / 3;

  if (column_count > entry_list->entry_count)
    column_count = entry_list->entry_count;

  if (column_count > ENTRY_LAYOUT_COLUMN_CAPACITY)
    column_count = ENTRY_LAYOUT_COLUMN_CAPACITY;

  if (column_count < 1)
    column_count = 1;

  for (;;) {
    layout->column_count = column_count;

    set_layout_column_byte_counts(entry_list, layout);

    if (column_count == 1)
      break;

    if (
      count_layout_row_bytes(layout) <=
      filer->terminal->line_byte_capacity
    )
      break;

    column_count--;
  }

  layout->row_count = entry_list->entry_count / layout->column_count;

  if (entry_list->entry_count % layout->column_count != 0)
    layout->row_count++;
}

static void
build_entry_row_text(
  const EntryList *entry_list,
  const EntryLayout *layout,
  int entry_row_index,
  char *row_text,
  int row_text_byte_capacity
) {

  int column_index = 0;
  int entry_index;
  char entry_label[NAME_MAX + 2];
  const char *entry_label_suffix = "";
  const char *column_separator = "";

  row_text[0] = 0;

  while (column_index < layout->column_count) {
    entry_index =
      entry_row_index * layout->column_count + column_index;

    if (entry_index >= entry_list->entry_count)
      return;

    if (entry_list->entries[entry_index].type == ENTRY_TYPE_DIR)
      entry_label_suffix = "/";
    else
      entry_label_suffix = "";

    if (column_index > 0)
      column_separator = "  ";
    else
      column_separator = "";

    snprintf(
      entry_label,
      sizeof entry_label,
      "%s%s",
      entry_list->entries[entry_index].name,
      entry_label_suffix
    );

    snprintf(
      row_text + strlen(row_text),
      row_text_byte_capacity - (int)strlen(row_text),
      "%s%-*s",
      column_separator,
      layout->column_byte_counts[column_index],
      entry_label
    );

    column_index++;
  }
}

static void
append_entry_rows_to_terminal_output(
  Filer *filer,
  const EntryList *entry_list,
  const EntryLayout *layout
) {

  int entry_row_index = 0;
  char row_text[LINE_MAX];

  while (entry_row_index < layout->row_count) {
    build_entry_row_text(
      entry_list,
      layout,
      entry_row_index,
      row_text,
      sizeof row_text
    );

    append_output_text(filer->terminal, row_text);

    entry_row_index++;
  }
}

static void
draw_entry_row(
  Filer *filer,
  const EntryList *entry_list,
  const EntryLayout *layout,
  int entry_row_index,
  int screen_row_index
) {
  char row_text[LINE_MAX];

  build_entry_row_text(
    entry_list,
    layout,
    entry_row_index,
    row_text,
    sizeof row_text
  );

  area512_sprite_fill(filer->row, area512_theme_background_color());

  area512_sprite_text(
    filer->row,
    TERMINAL_CONTENT_LEFT_X,
    0,
    row_text,
    area512_theme_text_color()
  );

  area512_sprite_push(filer->row, 0, screen_row_index * ROW_HEIGHT);
}

static void
draw_entry_rows(
  Filer *filer,
  const EntryList *entry_list,
  const EntryLayout *layout,
  int top_entry_row_index,
  int visible_screen_row_count
) {
  int screen_row_index = 0;

  if (!filer->row)
    return;

  while (
    screen_row_index < visible_screen_row_count &&
    top_entry_row_index + screen_row_index < layout->row_count
  ) {

    draw_entry_row(
      filer,
      entry_list,
      layout,
      top_entry_row_index + screen_row_index,
      screen_row_index
    );

    screen_row_index++;
  }
}

static void
run_entry_viewer(
  Filer *filer,
  const EntryList *entry_list,
  const EntryLayout *layout
) {

  int visible_screen_row_count = filer->height / ROW_HEIGHT;

  int maximum_top_entry_row_index =
    layout->row_count - visible_screen_row_count;

  int top_entry_row_index = 0;
  int input_key;

  if (maximum_top_entry_row_index < 0)
    maximum_top_entry_row_index = 0;

  area512_gfx_fill_screen(area512_theme_background_color());

  for (;;) {
    draw_entry_rows(
      filer,
      entry_list,
      layout,
      top_entry_row_index,
      visible_screen_row_count
    );

    input_key = read_terminal_key();

    if (
      input_key == 27 ||
      input_key == '\r' ||
      input_key == '\n' ||
      input_key == 'q'
    )
      break;

    if (input_key == KEY_UP && top_entry_row_index > 0)
      top_entry_row_index--;
    else if (
      input_key == KEY_DOWN &&
      top_entry_row_index < maximum_top_entry_row_index
    )
      top_entry_row_index++;
  }

  filer->full_redraw = 1;
}

static void
show_entry_list(Filer *filer, const EntryList *entry_list) {
  EntryLayout layout;

  if (entry_list->entry_count == 0)
    return;

  build_entry_layout(filer, entry_list, &layout);

  if (
    layout.row_count <=
    filer->terminal->visible_output_row_count
  ) {
    append_entry_rows_to_terminal_output(filer, entry_list, &layout);

    return;
  }

  run_entry_viewer(filer, entry_list, &layout);
}

static int
read_directory_entries(
  const char *absolute_path,
  FileEntry *entries,
  int entry_capacity,
  int *entry_count
) {

  char resolved_path[AREA512_PATH_MAX];
  DIR *directory_stream;
  struct dirent *directory_entry;
  FileEntry entry;

  *entry_count = 0;

  if (
    area512_resolve_data_path(
      absolute_path,
      resolved_path,
      sizeof resolved_path
    ) != 0
  )
    return 0;

  directory_stream = opendir(resolved_path);

  if (directory_stream == NULL)
    return 0;

  while ((directory_entry = readdir(directory_stream)) != NULL) {
    strncpy(entry.name, directory_entry->d_name, NAME_MAX - 1);
    entry.name[NAME_MAX - 1] = 0;

    entry.type =
      (directory_entry->d_type == DT_DIR)
        ? ENTRY_TYPE_DIR
        : ENTRY_TYPE_FILE;

    add_entry_in_order(entries, entry_count, entry_capacity, &entry);
  }

  closedir(directory_stream);

  return 1;
}

void
show_terminal_entries(Filer *filer) {
  EntryList entry_list;

  entry_list.entries = filer->entries;
  entry_list.entry_count = filer->count;

  show_entry_list(filer, &entry_list);
}

void
show_directory_entries(Filer *filer, const char *input_path) {
  char absolute_path[CURRENT_DIRECTORY_MAX];
  char message_text[MESSAGE_MAX];
  int target_is_directory;
  int select_status;
  FileEntry *entries;
  EntryList entry_list;

  select_status =
    resolve_target_path(
      filer->current_directory,
      input_path,
      absolute_path,
      sizeof absolute_path,
      &target_is_directory
    );

  if (select_status == TARGET_INVALID) {
    append_output_text(filer->terminal, "Bad path");

    return;
  }

  if (select_status == TARGET_MISSING) {
    snprintf(
      message_text,
      sizeof message_text,
      "No such directory: %s",
      input_path
    );

    append_output_text(filer->terminal, message_text);

    return;
  }

  if (!target_is_directory) {
    snprintf(
      message_text,
      sizeof message_text,
      "Not a directory: %s",
      input_path
    );

    append_output_text(filer->terminal, message_text);

    return;
  }

  entries = (FileEntry *)malloc(sizeof(FileEntry) * MAX_ENTRIES);

  if (entries == NULL)
    return;

  entry_list.entries = entries;

  if (
    read_directory_entries(
      absolute_path,
      entries,
      MAX_ENTRIES,
      &entry_list.entry_count
    )
  )
    show_entry_list(filer, &entry_list);

  free(entries);
}

#endif
