#include "core/register/search.h"
#include "core/render/footer.h"
#include "core/text/utf8.h"
#include <string.h>

static int
find_string_index(
  const char *haystack,
  int haystack_byte_length,
  const char *needle,
  int needle_byte_length,
  int from
) {

  if (from < 0)
    from = 0;

  if (needle_byte_length == 0) {
    if (from <= haystack_byte_length)
      return from;

    return -1;
  }

  for (int i = from; i + needle_byte_length <= haystack_byte_length; i++)
    if (memcmp(haystack + i, needle, (size_t)needle_byte_length) == 0)
      return i;

  return -1;
}

static int
find_last_string_index(
  const char *haystack,
  int haystack_byte_length,
  const char *needle,
  int needle_byte_length
) {

  if (needle_byte_length == 0)
    return haystack_byte_length;

  for (int i = haystack_byte_length - needle_byte_length; i >= 0; i--)
    if (memcmp(haystack + i, needle, (size_t)needle_byte_length) == 0)
      return i;

  return -1;
}

int
search_pattern(
  Vim *vim,
  const char *pattern,
  int pattern_byte_length,
  int forward,
  int show_not_found_message
) {

  if (pattern_byte_length == 0) {
    if (show_not_found_message)
      show_message_c_string(vim, "Pattern not found");

    return 0;
  }

  vim_string_set(&vim->search.last_search, pattern, pattern_byte_length);

  VimBuffer *buffer = BUFFER;

  if (forward) {
    int line_index = buffer->cursor_line_index;

    int byte_offset =
      buffer->cursor_byte_offset + vim_character_byte_length(
                                     buffer->lines[line_index].bytes,
                                     buffer->lines[line_index].byte_length,
                                     buffer->cursor_byte_offset
                                   );

    while (line_index < buffer->line_count) {
      int index = find_string_index(
        buffer->lines[line_index].bytes,
        buffer->lines[line_index].byte_length,
        pattern,
        pattern_byte_length,
        byte_offset
      );

      if (index >= 0) {
        vim_buffer_move_to(buffer, index, line_index);
        return 1;
      }

      line_index++;

      byte_offset = 0;
    }

    line_index = 0;
    while (line_index <= buffer->cursor_line_index) {
      int index = find_string_index(
        buffer->lines[line_index].bytes,
        buffer->lines[line_index].byte_length,
        pattern,
        pattern_byte_length,
        0
      );

      if (index >= 0) {
        vim_buffer_move_to(buffer, index, line_index);
        return 1;
      }

      line_index++;
    }

  } else {
    int line_index = buffer->cursor_line_index;
    int byte_offset = buffer->cursor_byte_offset - 1;

    if (byte_offset < 0)
      byte_offset = 0;

    while (line_index >= 0) {
      int prefix_byte_length = buffer->lines[line_index].byte_length;

      if (line_index == buffer->cursor_line_index)
        prefix_byte_length = byte_offset + 1;

      if (prefix_byte_length > buffer->lines[line_index].byte_length)
        prefix_byte_length = buffer->lines[line_index].byte_length;

      int index = find_last_string_index(
        buffer->lines[line_index].bytes,
        prefix_byte_length,
        pattern,
        pattern_byte_length
      );

      if (index >= 0) {
        vim_buffer_move_to(buffer, index, line_index);
        return 1;
      }

      line_index--;
    }

    line_index = buffer->line_count - 1;

    while (line_index >= buffer->cursor_line_index) {
      int index = find_last_string_index(
        buffer->lines[line_index].bytes,
        buffer->lines[line_index].byte_length,
        pattern,
        pattern_byte_length
      );

      if (index >= 0) {
        vim_buffer_move_to(buffer, index, line_index);
        return 1;
      }

      line_index--;
    }
  }

  if (show_not_found_message) {
    VimString message;
    vim_string_init(&message);

    vim_string_append_c_string(&message, "Pattern not found: ");
    vim_string_append(&message, pattern, pattern_byte_length);

    show_message(vim, message.bytes, message.byte_length);

    vim_string_free(&message);
  }

  return 0;
}

void
search_again(Vim *vim, int forward) {
  if (vim->search.last_search.byte_length > 0)
    search_pattern(
      vim,
      vim->search.last_search.bytes,
      vim->search.last_search.byte_length,
      forward,
      0
    );
}

void
repeat_find(Vim *vim, int reverse) {
  if (vim->search.last_find_char.byte_length == 0 ||
      vim->search.last_find_direction == 0)
    return;
  int direction = vim->search.last_find_direction;
  if (reverse)
    direction = -vim->search.last_find_direction;
  if (direction > 0)
    vim_buffer_find_char_forward(
      BUFFER,
      vim->search.last_find_char.bytes,
      vim->search.last_find_char.byte_length,
      vim->search.last_find_stop_before_match
    );
  else
    vim_buffer_find_char_backward(
      BUFFER,
      vim->search.last_find_char.bytes,
      vim->search.last_find_char.byte_length,
      vim->search.last_find_stop_before_match
    );
}
