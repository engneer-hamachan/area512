#ifndef EDIT_MODE_INSERT_H
#define EDIT_MODE_INSERT_H

#include "core/editor.h"

void handle_insert(
  Vim *vim,
  int key,
  const char *character,
  int character_byte_length
);
void enter_insert(Vim *vim);
void enter_insert_from_key(Vim *vim, int key);
void clear_current_line_and_enter_insert(Vim *vim);

#endif
