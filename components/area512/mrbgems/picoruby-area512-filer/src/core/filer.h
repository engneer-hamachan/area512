// Core filer declarations: layout constants, the filer state struct, and
// the ACTION_* codes run() returns to the Ruby shell (must stay in sync with
// the Ruby side). VM-independent: no mruby/c here.
#pragma once

#include <stdint.h>

#define FILER_CHAR_WIDTH 6
#define ROW_HEIGHT 13
#define HALF_ROW_HEIGHT 6
#define FILER_FONT_SIZE 12

#define NAME_MAX 40
#define MESSAGE_MAX 96
#define CURRENT_DIRECTORY_MAX 256
#define MAX_ENTRIES 256
#define LINE_MAX 96

#define ENTRY_TYPE_UP 0
#define ENTRY_TYPE_DIR 1
#define ENTRY_TYPE_FILE 2

#define ACTION_NONE 0
#define ACTION_OPEN_DIR 1
#define ACTION_UP 2
#define ACTION_RUN_RUBY 3
#define ACTION_COMPILE 4
#define ACTION_COMPILE_ALL 5
#define ACTION_RUN_DIR 6
#define ACTION_EDIT 7
#define ACTION_NEW_FILE 8
#define ACTION_NEW_DIR 9
#define ACTION_DELETE 10
#define ACTION_REBOOT 11
#define ACTION_MOVE 12
#define ACTION_VIEW_MARKDOWN 13
#define ACTION_RUN_PYTHON 14
#define ACTION_COPY 15
#define ACTION_EDIT_DOT 16
#define ACTION_CHANGE_DIR 17
#define ACTION_IRB 18

#define TARGET_RESOLVED 0
#define TARGET_INVALID 1
#define TARGET_MISSING 2

#define KEY_UP 1001
#define KEY_DOWN 1002
#define KEY_RIGHT 1003
#define KEY_LEFT 1004
#define KEY_COMPILE 1006
#define KEY_COMPILE_ALL 1007
#define KEY_EDIT 1008
#define KEY_RUN_DIR 1009
#define KEY_DELETE 1010
#define KEY_NEW_FILE 1011
#define KEY_NEW_DIR 1012
#define KEY_REBOOT 1013
#define KEY_MOVE 1014
#define KEY_COPY 1015
#define KEY_TERMINAL 1016

typedef struct {
  char name[NAME_MAX];
  uint8_t type;
} FileEntry;

typedef struct Terminal Terminal;

typedef struct {
  void *row;
  int width, height;
  int content_x, content_right, columns, edge_columns;
  int list_top, rows_visible, bar1_y, bar2_y, close_y;
  int panel_x, panel_right, list_columns_panel;
  int panel_top_row;
  int index, top, count, full_redraw;
  char current_directory[CURRENT_DIRECTORY_MAX];
  char message[MESSAGE_MAX];
  char input[LINE_MAX];
  char action_target_path[CURRENT_DIRECTORY_MAX];
  FileEntry entries[MAX_ENTRIES];
  Terminal *terminal;
} Filer;

void init_filer_state(Filer *filer);
void clamp_index(Filer *filer);
void move_cursor(Filer *filer, int delta);
void jump_to(Filer *filer, int offset);
FileEntry *fetch_selected_entry(Filer *filer);

void add_entry_in_order(
  FileEntry *entries,
  int *entry_count,
  int entry_capacity,
  const FileEntry *entry
);

int is_ruby_file_path(const char *file_path);
int is_python_file_path(const char *file_path);
int is_markdown_file_path(const char *file_path);
int is_dot_image_file_path(const char *file_path);
int is_source_file_path(const char *file_path);
int is_editable_file_path(const char *file_path);

int is_selected_editable(Filer *filer);
int is_selected_markdown_file(Filer *filer);
int is_selected_ruby_file(Filer *filer);
int is_selected_python_file(Filer *filer);
int is_selected_source_file(Filer *filer);
int is_selected_dot_image_file(Filer *filer);

void fit_string(
  char *destination,
  int destination_size,
  const char *source,
  int width
);

int area512_filer_read_key(void);
int read_raw_text_key(void);
int read_terminal_key(void);
void area512_filer_setup_ui(Filer *filer);
void area512_filer_teardown_ui(Filer *filer);

void build_delete_question(
  Filer *filer,
  const char *target_label,
  int target_is_directory,
  char *question,
  int question_size
);

int read_text_input(Filer *filer, const char *label);
int read_yes_no_confirmation(Filer *filer, const char *question);
int run_filer_interaction(Filer *filer);
void draw_all(Filer *filer);
