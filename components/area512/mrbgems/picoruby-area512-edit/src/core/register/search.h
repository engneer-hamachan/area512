#ifndef EDIT_REGISTER_SEARCH_H
#define EDIT_REGISTER_SEARCH_H

#include "core/editor.h"

int search_pattern(
  Vim *vim,
  const char *pattern,
  int pattern_byte_length,
  int forward,
  int show_not_found_message
);
void search_again(Vim *vim, int forward);
void repeat_find(Vim *vim, int reverse);

#endif
