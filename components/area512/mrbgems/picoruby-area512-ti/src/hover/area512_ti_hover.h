#ifndef AREA512_TI_HOVER_H
#define AREA512_TI_HOVER_H

#define TI_TYPE_NAME_CAPACITY 96
#define TI_VARIABLE_NAME_CAPACITY 64

typedef struct {
  char variable_name[TI_VARIABLE_NAME_CAPACITY];
  char type_name[TI_TYPE_NAME_CAPACITY];
  int found;
} TiTypeInfo;

int ti_find_type_at_cursor(
  const char *source,
  int source_byte_length,
  int cursor_byte_offset,
  TiTypeInfo *out
);

#endif
