#ifndef EDIT_TEXT_STRING_H
#define EDIT_TEXT_STRING_H

typedef struct {
  char *bytes;
  int byte_length;
  int capacity;
} VimString;

void vim_string_init(VimString *string);
void vim_string_free(VimString *string);
void vim_string_clear(VimString *string);
int vim_string_append(VimString *string, const char *text, int byte_length);
int vim_string_append_c_string(VimString *string, const char *text);
int vim_string_append_byte(VimString *string, char byte);
int vim_string_append_integer(VimString *string, int value);
int vim_string_set(VimString *string, const char *text, int byte_length);
int vim_string_append_slice(
  VimString *string,
  const char *source,
  int source_byte_length,
  int from,
  int to
);

#endif
