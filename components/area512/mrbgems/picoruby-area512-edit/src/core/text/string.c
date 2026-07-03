#include "core/text/string.h"
#include <stdlib.h>
#include <string.h>

void
vim_string_init(VimString *string) {
  string->bytes = NULL;
  string->byte_length = 0;
  string->capacity = 0;
}

void
vim_string_free(VimString *string) {
  if (string->bytes)
    free(string->bytes);

  string->bytes = NULL;
  string->byte_length = 0;
  string->capacity = 0;
}

void
vim_string_clear(VimString *string) {
  string->byte_length = 0;
}

static int
vim_string_reserve(VimString *string, int needed_byte_length) {
  int required_byte_length = string->byte_length + needed_byte_length;
  if (required_byte_length <= string->capacity)
    return 1;

  int new_capacity = 16;
  if (string->capacity)
    new_capacity = string->capacity * 2;

  while (new_capacity < required_byte_length)
    new_capacity *= 2;

  char *new_bytes = (char *)realloc(string->bytes, (size_t)new_capacity);

  if (!new_bytes)
    return 0;

  string->bytes = new_bytes;
  string->capacity = new_capacity;

  return 1;
}

int
vim_string_append(VimString *string, const char *text, int byte_length) {
  if (byte_length <= 0)
    return 1;

  if (!vim_string_reserve(string, byte_length))
    return 0;

  memcpy(string->bytes + string->byte_length, text, (size_t)byte_length);

  string->byte_length += byte_length;

  return 1;
}

int
vim_string_append_c_string(VimString *string, const char *text) {
  return vim_string_append(string, text, (int)strlen(text));
}

int
vim_string_append_byte(VimString *string, char byte) {
  if (!vim_string_reserve(string, 1))
    return 0;

  string->bytes[string->byte_length++] = byte;

  return 1;
}

int
vim_string_append_integer(VimString *string, int value) {
  char digits[16];

  int i = 0;

  unsigned int magnitude;

  if (value < 0) {
    vim_string_append_byte(string, '-');
    magnitude = (unsigned int)(-(value + 1)) + 1u;
  } else
    magnitude = (unsigned int)value;

  if (magnitude == 0)
    digits[i++] = '0';

  while (magnitude) {
    digits[i++] = (char)('0' + (magnitude % 10));
    magnitude /= 10;
  }

  while (i > 0) {
    if (!vim_string_append_byte(string, digits[--i]))
      return 0;
  }

  return 1;
}

int
vim_string_set(VimString *string, const char *text, int byte_length) {
  string->byte_length = 0;
  return vim_string_append(string, text, byte_length);
}

int
vim_string_append_slice(
  VimString *string,
  const char *source,
  int source_byte_length,
  int from,
  int to
) {

  if (from < 0)
    from = 0;

  if (to > source_byte_length)
    to = source_byte_length;

  if (to <= from)
    return 1;

  return vim_string_append(string, source + from, to - from);
}
