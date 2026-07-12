#ifndef EDIT_EDITOR_H
#define EDIT_EDITOR_H

#include "core/render/screen.h"

#define INDENT_UNIT "  "
#define INDENT_UNIT_BYTE_LENGTH 2
#define BUFFER (&vim->screen.buffer)
#define REDRAW(mode) (vim->screen.redraw_mode = (mode))

typedef enum {
  VIM_MODE_NORMAL = 0,
  VIM_MODE_OPERATOR,
  VIM_MODE_COMMAND,
  VIM_MODE_SEARCH,
  VIM_MODE_INSERT,
  VIM_MODE_VISUAL,
  VIM_MODE_VISUAL_LINE,
  VIM_MODE_VISUAL_BLOCK
} VimMode;

typedef enum {
  VIM_OPERATOR_NONE = 0,
  VIM_OPERATOR_DELETE,
  VIM_OPERATOR_CHANGE
} VimOperator;

typedef enum {
  VIM_LAST_CHANGE_NONE = 0,
  VIM_LAST_CHANGE_DELETE_CHAR,
  VIM_LAST_CHANGE_REPLACE,
  VIM_LAST_CHANGE_PASTE,
  VIM_LAST_CHANGE_DELETE_EOL,
  VIM_LAST_CHANGE_CHANGE_EOL,
  VIM_LAST_CHANGE_JOIN,
  VIM_LAST_CHANGE_INDENT,
  VIM_LAST_CHANGE_OUTDENT,
  VIM_LAST_CHANGE_TOGGLE_CASE,
  VIM_LAST_CHANGE_CHANGE_LINE,
  VIM_LAST_CHANGE_OPERATOR,
  VIM_LAST_CHANGE_OPEN_BELOW,
  VIM_LAST_CHANGE_OPEN_ABOVE,
  VIM_LAST_CHANGE_SUBSTITUTE
} VimLastChange;

typedef enum {
  VIM_PENDING_NONE = 0,
  VIM_PENDING_REPLACE,
  VIM_PENDING_FIND,
  VIM_PENDING_G,
  VIM_PENDING_OUTDENT,
  VIM_PENDING_INDENT,
  VIM_PENDING_YANK
} VimPending;

typedef enum { VIM_CONTINUE = 0, VIM_QUIT, VIM_SAVE, VIM_SAVE_QUIT } VimStatus;

typedef struct {
  VimMode mode;
  VimOperator current_operator;

  int has_count;
  int count;

  int pending;
  int pending_direction;
  int pending_stop_before_match;
  int pending_count;
  int pending_has_count;
} VimInput;

typedef struct {
  VimString command;
  VimString message;
  int has_message;
} VimStatusLine;

typedef struct {
  VimString last_search;
  VimString last_find_char;
  int last_find_direction;
  int last_find_stop_before_match;
} VimSearch;

typedef struct {
  VimString text;
  int is_line;
} VimPaste;

typedef struct {
  VimLastChange last_change;
  int last_change_paste_after_cursor;
  VimOperator last_change_operator;
  int last_change_motion;
  char last_change_character[8];
  int last_change_character_byte_length;
} VimRepeat;

typedef struct {
  VimScreen screen;
  VimCanvas *active_canvas;
  VimInput input;
  VimStatusLine status;
  VimSearch search;
  VimPaste paste;
  VimRepeat repeat;
  VimString filepath;
} Vim;

void vim_init(Vim *vim, int width, int height);
void vim_free(Vim *vim);
void vim_set_filepath(Vim *vim, const char *text, int byte_length);
void vim_load_text(Vim *vim, const char *text, int byte_length);

VimStatus vim_handle_key(
  Vim *vim,
  int first_byte,
  const char *character,
  int character_byte_length
);
VimStatus vim_handle_esc(Vim *vim, const char *sequence, int byte_length);

void vim_write_content(Vim *vim, VimString *content);
void vim_handle_after_save(Vim *vim, int saved);

#endif
