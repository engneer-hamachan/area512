#ifndef EDIT_BUFFER_BUFFER_H
#define EDIT_BUFFER_BUFFER_H

#include "core/text/string.h"
typedef enum {
  VIM_DIRTY_NONE = 0,
  VIM_DIRTY_CURSOR,
  VIM_DIRTY_CONTENT,
  VIM_DIRTY_STRUCTURE
} VimDirty;

typedef enum {
  VIM_SELECTION_NONE = 0,
  VIM_SELECTION_CHAR,
  VIM_SELECTION_LINE
} VimSelectionMode;

typedef struct {
  VimString *lines;
  int line_count;
  int capacity;

  int cursor_byte_offset;
  int cursor_line_index;

  int changed;
  VimDirty dirty;

  VimSelectionMode selection_mode;
  int selection_start_byte_offset;
  int selection_start_line_index;
} VimBuffer;

void vim_buffer_init(VimBuffer *buffer);
void vim_buffer_free(VimBuffer *buffer);
void vim_buffer_clear(VimBuffer *buffer);
void vim_buffer_load_text(VimBuffer *buffer, const char *text, int byte_length);

static inline VimString *
vim_buffer_current_line(VimBuffer *buffer) {
  return &buffer->lines[buffer->cursor_line_index];
}

static inline int
vim_buffer_current_line_byte_length(VimBuffer *buffer) {
  return buffer->lines[buffer->cursor_line_index].byte_length;
}

void vim_buffer_mark_dirty(VimBuffer *buffer, VimDirty level);
void vim_buffer_clear_dirty(VimBuffer *buffer);

int vim_buffer_insert_line(
  VimBuffer *buffer,
  int line_index,
  const char *text,
  int byte_length
);
int vim_buffer_set_line(
  VimBuffer *buffer,
  int line_index,
  const char *text,
  int byte_length
);
void vim_buffer_delete_line_at(VimBuffer *buffer, int line_index);
void vim_buffer_set_line_built(
  VimBuffer *buffer,
  int line_index,
  VimString *built_line
);

void vim_buffer_move_home(VimBuffer *buffer);
void vim_buffer_move_to_line_head(VimBuffer *buffer);
void vim_buffer_move_to_line_tail(VimBuffer *buffer);
void vim_buffer_move_up(VimBuffer *buffer);
void vim_buffer_move_down(VimBuffer *buffer);
void vim_buffer_move_left(VimBuffer *buffer);
void vim_buffer_move_right(VimBuffer *buffer);
void vim_buffer_clamp_cursor(VimBuffer *buffer);
void vim_buffer_move_to(VimBuffer *buffer, int byte_offset, int line_index);
void vim_buffer_move_to_line_first_nonblank(VimBuffer *buffer);
void vim_buffer_move_to_line_end(VimBuffer *buffer);
void
vim_buffer_go_to_line(VimBuffer *buffer, int line_index, int first_nonblank);
void vim_buffer_move_to_first_line(VimBuffer *buffer);
void vim_buffer_move_to_last_line(VimBuffer *buffer);

typedef enum {
  VIM_PUT_TAB = 1,
  VIM_PUT_ENTER,
  VIM_PUT_BACKSPACE,
  VIM_PUT_DOWN,
  VIM_PUT_UP,
  VIM_PUT_RIGHT,
  VIM_PUT_LEFT,
  VIM_PUT_HOME
} VimPutKey;

void
vim_buffer_put_string(VimBuffer *buffer, const char *text, int byte_length);
void vim_buffer_put_key(VimBuffer *buffer, VimPutKey key);

void vim_buffer_delete_char(VimBuffer *buffer);
int vim_buffer_delete_line(VimBuffer *buffer, VimString *yanked_text);
void vim_buffer_replace_char(
  VimBuffer *buffer,
  const char *character,
  int byte_length
);
int vim_buffer_delete_range(
  VimBuffer *buffer,
  int start_line_index,
  int start_byte_offset,
  int end_line_index,
  int end_byte_offset,
  VimString *yanked_text
);
int vim_buffer_delete_to_eol(VimBuffer *buffer, VimString *yanked_text);
int vim_buffer_join_next_line(VimBuffer *buffer);
void vim_buffer_indent_line(
  VimBuffer *buffer,
  const char *indent,
  int indent_byte_length
);
void vim_buffer_indent_line_at(
  VimBuffer *buffer,
  int line_index,
  const char *indent,
  int indent_byte_length
);
void vim_buffer_outdent_line(
  VimBuffer *buffer,
  const char *indent,
  int indent_byte_length
);
void vim_buffer_outdent_line_at(
  VimBuffer *buffer,
  int line_index,
  const char *indent,
  int indent_byte_length
);
void vim_buffer_toggle_case(VimBuffer *buffer);

void vim_buffer_insert_after_cursor(
  VimBuffer *buffer,
  const char *text,
  int byte_length
);
void vim_buffer_insert_before_cursor(
  VimBuffer *buffer,
  const char *text,
  int byte_length
);
void vim_buffer_insert_lines_below(
  VimBuffer *buffer,
  const char *text,
  int byte_length
);
void vim_buffer_insert_lines_above(
  VimBuffer *buffer,
  const char *text,
  int byte_length
);

void vim_buffer_move_to_next_word(VimBuffer *buffer);
void vim_buffer_move_to_previous_word(VimBuffer *buffer);
void vim_buffer_move_to_word_end(VimBuffer *buffer);
void vim_buffer_move_to_next_word_group(VimBuffer *buffer);
void vim_buffer_move_to_previous_word_group(VimBuffer *buffer);
void vim_buffer_move_to_word_group_end(VimBuffer *buffer);
void vim_buffer_move_to_next_paragraph(VimBuffer *buffer);
void vim_buffer_move_to_previous_paragraph(VimBuffer *buffer);

int vim_buffer_find_char_forward(
  VimBuffer *buffer,
  const char *character,
  int character_byte_length,
  int stop_before_match
);
int vim_buffer_find_char_backward(
  VimBuffer *buffer,
  const char *character,
  int character_byte_length,
  int stop_after_match
);
int vim_buffer_find_matching_pair(VimBuffer *buffer);

void vim_buffer_start_selection(VimBuffer *buffer, VimSelectionMode mode);
void vim_buffer_clear_selection(VimBuffer *buffer);
int vim_buffer_has_selection(VimBuffer *buffer);
int vim_buffer_selection_range(
  VimBuffer *buffer,
  int *start_line_index,
  int *start_byte_offset,
  int *end_line_index,
  int *end_byte_offset
);
int vim_buffer_copy_selected_text(VimBuffer *buffer, VimString *yanked_text);
int vim_buffer_delete_selected_text(VimBuffer *buffer, VimString *yanked_text);

#endif
