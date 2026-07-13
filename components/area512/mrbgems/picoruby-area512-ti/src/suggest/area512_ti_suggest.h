#ifndef AREA512_TI_SUGGEST_H
#define AREA512_TI_SUGGEST_H

#include "area512_ti_context.h"
#include "area512_ti_eval.h"

int ti_collect_suggestions(
  TiContext *context,
  const pm_node_t *root,
  int cursor_byte_offset,
  TiSuggestionList *out
);

#endif
