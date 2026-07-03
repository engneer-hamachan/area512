#ifndef EDIT_MODE_NORMAL_H
#define EDIT_MODE_NORMAL_H

#include "core/editor.h"

VimStatus handle_normal(
  Vim *vim,
  int key,
  const char *character,
  int character_byte_length
);

#endif
