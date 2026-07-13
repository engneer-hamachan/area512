#ifndef AREA512_TI_EVAL_H
#define AREA512_TI_EVAL_H

#define TI_MAX_SUGGESTIONS 64

typedef struct {
  const char *contents;
  int contents_length;
  const char *detail;
  const char *document;
  const char *class_name;
} TiSuggestion;

typedef struct {
  TiSuggestion items[TI_MAX_SUGGESTIONS];
  int count;
} TiSuggestionList;

int ti_fill_suggestions_at_cursor(
  const char *source,
  int source_byte_length,
  int cursor_byte_offset,
  TiSuggestionList *out
);

#endif
