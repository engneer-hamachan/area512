#ifndef AREA512_EDIT_COMPLETE_POPUP_H
#define AREA512_EDIT_COMPLETE_POPUP_H

#include "area512_ti_eval.h"
#include "core/editor.h"

int show_complete_popup(
  Vim *vim,
  const TiSuggestionList *suggestions,
  int *next_key,
  char next_character[4],
  int *next_character_byte_length
);

#endif
