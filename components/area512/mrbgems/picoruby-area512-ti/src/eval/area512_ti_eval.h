#ifndef AREA512_TI_EVAL_H
#define AREA512_TI_EVAL_H

#define TI_MAX_SUGGESTIONS 64
#define TI_TYPE_NAME_CAPACITY 96
#define TI_VARIABLE_NAME_CAPACITY 64

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

typedef struct {
  char variable_name[TI_VARIABLE_NAME_CAPACITY];
  char type_name[TI_TYPE_NAME_CAPACITY];
  int found;
} TiTypeInfo;

int ti_fill_suggestions_at_cursor(
  const char *source,
  int source_byte_length,
  int cursor_byte_offset,
  TiSuggestionList *out
);

int ti_find_type_at_cursor(
  const char *source,
  int source_byte_length,
  int cursor_byte_offset,
  TiTypeInfo *out
);

#endif
