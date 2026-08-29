// Terminal mode: a command line over the same Filer state and ACTION_* codes
// the list UI uses. Entered with KEY_TERMINAL, left with the exit command.
#pragma once

#include "core/filer.h"

#define TERMINAL_OUTPUT_ROW_CAPACITY 10
#define TERMINAL_HISTORY_LINE_CAPACITY 5
#define TERMINAL_CONTENT_LEFT_X 2
#define TERMINAL_HOME_DIRECTORY "/home"
#define TERMINAL_APPEND_AUTOSUGGESTION_KEY ('F' & 0x1F)

// Returned by execute_terminal_command for the exit command only.
#define TERMINAL_ACTION_EXIT (-1)

struct Terminal {
  char output_lines[TERMINAL_OUTPUT_ROW_CAPACITY][LINE_MAX];
  int visible_output_row_count;
  int output_row_count;
  int line_byte_capacity;
  char input_line[LINE_MAX];
  int input_byte_count;
  int input_cursor_byte_offset;
  int visible_input_byte_offset;
  char history_lines[TERMINAL_HISTORY_LINE_CAPACITY][LINE_MAX];
  int history_line_count;
  int history_recall_index;
  char completion_word[LINE_MAX];
  int completion_match_index;
  int entry_index_to_restore;
};

void open_terminal_session(Filer *filer);
int run_terminal_session(Filer *filer);
void close_terminal_session(Filer *filer);

void draw_terminal(Filer *filer);

void append_output_text(Terminal *terminal, const char *text);
void clear_output_lines(Terminal *terminal);

void read_terminal_input_line(Filer *filer);
void complete_input_word(Filer *filer);
void build_autosuggestion(
  Filer *filer,
  char *autosuggestion,
  int autosuggestion_capacity
);

int execute_terminal_command(Filer *filer);
const char *fetch_command_name(int command_index);

void show_terminal_entries(Filer *filer);
void show_directory_entries(Filer *filer, const char *input_path);
void show_metrics(Filer *filer);

void expand_tilde(
  const char *input_path,
  char *expanded_path,
  int expanded_path_capacity
);

int resolve_target_path(
  const char *current_directory,
  const char *input_path,
  char *absolute_path,
  int absolute_path_capacity,
  int *target_is_directory
);

int parent_path_is_directory(const char *absolute_path);

void append_history_line(Terminal *terminal, const char *input_line);
