#include "core/buffer/buffer.h"
#include "core/text/utf8.h"
#include <string.h>

static int
character_matches_text(
  const char *text,
  int byte_length,
  int byte_offset,
  const char *literal
) {

  int literal_byte_length = (int)strlen(literal);
  int character_byte_length =
    vim_character_byte_length(text, byte_length, byte_offset);

  if (character_byte_length != literal_byte_length)
    return 0;

  return memcmp(text + byte_offset, literal, (size_t)literal_byte_length) == 0;
}

static int
is_word_character(const char *text, int byte_length, int byte_offset) {
  int character_byte_length =
    vim_character_byte_length(text, byte_length, byte_offset);

  if (character_byte_length != 1)
    return 0;

  unsigned char character = (unsigned char)text[byte_offset];

  return (character >= '0' && character <= '9') ||
         (character >= 'A' && character <= 'Z') ||
         (character >= 'a' && character <= 'z') || character == '_';
}

static int
is_blank_line(const VimString *line) {
  for (int i = 0; i < line->byte_length; i++) {
    char character = line->bytes[i];

    if (character != ' ' && character != '\t' && character != '\r' &&
        character != '\f' && character != '\v')
      return 0;
  }

  return 1;
}

void
vim_buffer_move_home(VimBuffer *buffer) {
  buffer->cursor_byte_offset = 0;
  buffer->cursor_line_index = 0;

  vim_buffer_mark_dirty(buffer, VIM_DIRTY_CURSOR);
}

void
vim_buffer_move_to_line_head(VimBuffer *buffer) {
  buffer->cursor_byte_offset = 0;
  vim_buffer_mark_dirty(buffer, VIM_DIRTY_CURSOR);
}

void
vim_buffer_move_to_line_tail(VimBuffer *buffer) {
  buffer->cursor_byte_offset = vim_buffer_current_line_byte_length(buffer);
  vim_buffer_mark_dirty(buffer, VIM_DIRTY_CURSOR);
}

void
vim_buffer_move_up(VimBuffer *buffer) {
  if (buffer->cursor_line_index > 0) {
    buffer->cursor_line_index--;
    vim_buffer_mark_dirty(buffer, VIM_DIRTY_CURSOR);
  }
}

void
vim_buffer_move_down(VimBuffer *buffer) {
  if (buffer->cursor_line_index + 1 < buffer->line_count) {
    buffer->cursor_line_index++;
    vim_buffer_mark_dirty(buffer, VIM_DIRTY_CURSOR);
  }
}

void
vim_buffer_move_left(VimBuffer *buffer) {
  VimString *line = vim_buffer_current_line(buffer);
  if (buffer->cursor_byte_offset > 0 && line->byte_length > 0) {
    if (line->byte_length < buffer->cursor_byte_offset)
      buffer->cursor_byte_offset = line->byte_length;

    buffer->cursor_byte_offset =
      vim_previous_character_byte_offset(line->bytes, buffer->cursor_byte_offset);
    vim_buffer_mark_dirty(buffer, VIM_DIRTY_CURSOR);

  } else if (buffer->cursor_line_index > 0) {
    vim_buffer_move_up(buffer);
    vim_buffer_move_to_line_tail(buffer);
  }
}

void
vim_buffer_move_right(VimBuffer *buffer) {
  VimString *line = vim_buffer_current_line(buffer);

  if (buffer->cursor_byte_offset < line->byte_length) {
    buffer->cursor_byte_offset += vim_character_byte_length(
      line->bytes,
      line->byte_length,
      buffer->cursor_byte_offset
    );

    vim_buffer_mark_dirty(buffer, VIM_DIRTY_CURSOR);

  } else if (buffer->cursor_line_index + 1 < buffer->line_count) {
    vim_buffer_move_down(buffer);
    vim_buffer_move_to_line_head(buffer);
  }
}

void
vim_buffer_move_to(VimBuffer *buffer, int byte_offset, int line_index) {
  buffer->cursor_byte_offset = byte_offset;
  buffer->cursor_line_index = line_index;

  vim_buffer_mark_dirty(buffer, VIM_DIRTY_CURSOR);
}

void
vim_buffer_clamp_cursor(VimBuffer *buffer) {
  if (buffer->cursor_line_index < 0)
    buffer->cursor_line_index = 0;

  if (buffer->cursor_line_index >= buffer->line_count)
    buffer->cursor_line_index = buffer->line_count - 1;

  if (buffer->cursor_line_index < 0)
    buffer->cursor_line_index = 0;

  VimString *line = vim_buffer_current_line(buffer);

  if (buffer->cursor_byte_offset < 0)
    buffer->cursor_byte_offset = 0;

  if (buffer->cursor_byte_offset > line->byte_length)
    buffer->cursor_byte_offset = line->byte_length;

  vim_buffer_mark_dirty(buffer, VIM_DIRTY_CURSOR);
}

void
vim_buffer_move_to_line_first_nonblank(VimBuffer *buffer) {
  VimString *line = vim_buffer_current_line(buffer);

  int byte_offset = 0;

  while (byte_offset < line->byte_length) {
    if (!(character_matches_text(
            line->bytes,
            line->byte_length,
            byte_offset,
            " "
          ) ||
          character_matches_text(
            line->bytes,
            line->byte_length,
            byte_offset,
            "\t"
          )))
      break;

    byte_offset +=
      vim_character_byte_length(line->bytes, line->byte_length, byte_offset);
  }

  buffer->cursor_byte_offset = byte_offset;
  vim_buffer_mark_dirty(buffer, VIM_DIRTY_CURSOR);
}

void
vim_buffer_move_to_line_end(VimBuffer *buffer) {
  VimString *line = vim_buffer_current_line(buffer);

  buffer->cursor_byte_offset = 0;
  if (line->byte_length > 0)
    buffer->cursor_byte_offset =
      vim_previous_character_byte_offset(line->bytes, line->byte_length);

  vim_buffer_mark_dirty(buffer, VIM_DIRTY_CURSOR);
}

void
vim_buffer_go_to_line(VimBuffer *buffer, int line_index, int first_nonblank) {

  if (buffer->line_count == 0)
    return;

  if (line_index < 0)
    line_index = 0;

  if (line_index >= buffer->line_count)
    line_index = buffer->line_count - 1;

  buffer->cursor_line_index = line_index;
  buffer->cursor_byte_offset = 0;

  if (first_nonblank)
    vim_buffer_move_to_line_first_nonblank(buffer);
  else
    vim_buffer_clamp_cursor(buffer);
}

void
vim_buffer_move_to_first_line(VimBuffer *buffer) {
  vim_buffer_go_to_line(buffer, 0, 1);
}

void
vim_buffer_move_to_last_line(VimBuffer *buffer) {
  vim_buffer_go_to_line(buffer, buffer->line_count - 1, 1);
}

void
vim_buffer_move_to_next_word(VimBuffer *buffer) {
  VimString *line = vim_buffer_current_line(buffer);

  int byte_length = line->byte_length;

  if (buffer->cursor_byte_offset >= byte_length) {
    if (buffer->cursor_line_index + 1 < buffer->line_count) {
      vim_buffer_move_down(buffer);
      vim_buffer_move_to_line_head(buffer);
    }

    return;
  }

  int byte_offset = buffer->cursor_byte_offset;

  if (is_word_character(line->bytes, byte_length, byte_offset)) {
    while (byte_offset < byte_length &&
           is_word_character(line->bytes, byte_length, byte_offset))
      byte_offset +=
        vim_character_byte_length(line->bytes, byte_length, byte_offset);

  } else if (!character_matches_text(
               line->bytes,
               byte_length,
               byte_offset,
               " "
             )) {
    while (byte_offset < byte_length &&
           !is_word_character(line->bytes, byte_length, byte_offset) &&
           !character_matches_text(line->bytes, byte_length, byte_offset, " "))
      byte_offset +=
        vim_character_byte_length(line->bytes, byte_length, byte_offset);
  }

  while (byte_offset < byte_length &&
         character_matches_text(line->bytes, byte_length, byte_offset, " "))
    byte_offset +=
      vim_character_byte_length(line->bytes, byte_length, byte_offset);

  if (byte_offset >= byte_length &&
      buffer->cursor_line_index + 1 < buffer->line_count) {

    vim_buffer_move_down(buffer);
    vim_buffer_move_to_line_head(buffer);

  } else {
    if (byte_offset < byte_length)
      buffer->cursor_byte_offset = byte_offset;
    else {
      buffer->cursor_byte_offset = 0;

      if (byte_length > 0)
        buffer->cursor_byte_offset =
          vim_previous_character_byte_offset(line->bytes, byte_length);
    }

    vim_buffer_mark_dirty(buffer, VIM_DIRTY_CURSOR);
  }
}

void
vim_buffer_move_to_previous_word(VimBuffer *buffer) {
  if (buffer->cursor_byte_offset == 0) {
    if (buffer->cursor_line_index > 0) {
      vim_buffer_move_up(buffer);
      vim_buffer_move_to_line_tail(buffer);

      VimString *line = vim_buffer_current_line(buffer);

      buffer->cursor_byte_offset = 0;

      if (line->byte_length > 0)
        buffer->cursor_byte_offset =
          vim_previous_character_byte_offset(line->bytes, line->byte_length);

      vim_buffer_mark_dirty(buffer, VIM_DIRTY_CURSOR);
    }

    return;
  }

  VimString *line = vim_buffer_current_line(buffer);

  int byte_offset =
    vim_previous_character_byte_offset(line->bytes, buffer->cursor_byte_offset);

  while (
    byte_offset > 0 &&
    character_matches_text(line->bytes, line->byte_length, byte_offset, " ")
  )
    byte_offset = vim_previous_character_byte_offset(line->bytes, byte_offset);

  if (is_word_character(line->bytes, line->byte_length, byte_offset)) {
    while (byte_offset > 0) {
      int previous_byte_offset =
        vim_previous_character_byte_offset(line->bytes, byte_offset);

      if (!is_word_character(
            line->bytes,
            line->byte_length,
            previous_byte_offset
          ))
        break;

      byte_offset = previous_byte_offset;
    }
  } else if (byte_offset > 0) {
    while (byte_offset > 0) {
      int previous_byte_offset =
        vim_previous_character_byte_offset(line->bytes, byte_offset);

      if (is_word_character(
            line->bytes,
            line->byte_length,
            previous_byte_offset
          ) ||
          character_matches_text(
            line->bytes,
            line->byte_length,
            previous_byte_offset,
            " "
          ))
        break;

      byte_offset = previous_byte_offset;
    }
  }

  buffer->cursor_byte_offset = byte_offset;
  vim_buffer_mark_dirty(buffer, VIM_DIRTY_CURSOR);
}

void
vim_buffer_move_to_word_end(VimBuffer *buffer) {
  VimString *line = vim_buffer_current_line(buffer);
  int byte_length = line->byte_length;

  int last_character_byte_offset = 0;
  if (byte_length > 0)
    last_character_byte_offset =
      vim_previous_character_byte_offset(line->bytes, byte_length);

  if (buffer->cursor_byte_offset >= last_character_byte_offset) {
    if (buffer->cursor_line_index + 1 < buffer->line_count) {
      vim_buffer_move_down(buffer);
      line = vim_buffer_current_line(buffer);
      byte_length = line->byte_length;
      buffer->cursor_byte_offset = 0;
    } else
      return;

  } else {
    buffer->cursor_byte_offset += vim_character_byte_length(
      line->bytes,
      byte_length,
      buffer->cursor_byte_offset
    );
  }

  int byte_offset = buffer->cursor_byte_offset;

  while (byte_offset < byte_length &&
         character_matches_text(line->bytes, byte_length, byte_offset, " "))
    byte_offset +=
      vim_character_byte_length(line->bytes, byte_length, byte_offset);

  if (is_word_character(line->bytes, byte_length, byte_offset)) {
    while (byte_offset < byte_length &&
           is_word_character(line->bytes, byte_length, byte_offset))
      byte_offset +=
        vim_character_byte_length(line->bytes, byte_length, byte_offset);
  } else {
    while (byte_offset < byte_length &&
           !is_word_character(line->bytes, byte_length, byte_offset) &&
           !character_matches_text(line->bytes, byte_length, byte_offset, " "))
      byte_offset +=
        vim_character_byte_length(line->bytes, byte_length, byte_offset);
  }

  buffer->cursor_byte_offset = 0;
  if (byte_offset > 0)
    buffer->cursor_byte_offset =
      vim_previous_character_byte_offset(line->bytes, byte_offset);

  vim_buffer_mark_dirty(buffer, VIM_DIRTY_CURSOR);
}

void
vim_buffer_move_to_next_word_group(VimBuffer *buffer) {
  VimString *line = vim_buffer_current_line(buffer);

  int byte_offset = buffer->cursor_byte_offset;

  byte_offset +=
    vim_character_byte_length(line->bytes, line->byte_length, byte_offset);

  for (;;) {
    while (byte_offset >= line->byte_length) {
      if (buffer->cursor_line_index + 1 >= buffer->line_count) {
        vim_buffer_move_to_line_end(buffer);
        return;
      }

      buffer->cursor_line_index++;

      line = vim_buffer_current_line(buffer);
      byte_offset = 0;
    }

    if (!character_matches_text(
          line->bytes,
          line->byte_length,
          byte_offset,
          " "
        ))
      break;

    byte_offset +=
      vim_character_byte_length(line->bytes, line->byte_length, byte_offset);
  }

  while (
    byte_offset < line->byte_length &&
    !character_matches_text(line->bytes, line->byte_length, byte_offset, " ")
  )

    byte_offset +=
      vim_character_byte_length(line->bytes, line->byte_length, byte_offset);

  while (
    byte_offset < line->byte_length &&
    character_matches_text(line->bytes, line->byte_length, byte_offset, " ")
  )

    byte_offset +=
      vim_character_byte_length(line->bytes, line->byte_length, byte_offset);

  if (byte_offset >= line->byte_length &&
      buffer->cursor_line_index + 1 < buffer->line_count) {

    buffer->cursor_line_index++;
    buffer->cursor_byte_offset = 0;

  } else {
    if (byte_offset < line->byte_length) {
      buffer->cursor_byte_offset = byte_offset;
    } else {
      buffer->cursor_byte_offset = 0;

      if (line->byte_length > 0)
        buffer->cursor_byte_offset =
          vim_previous_character_byte_offset(line->bytes, line->byte_length);
    }
  }

  vim_buffer_mark_dirty(buffer, VIM_DIRTY_CURSOR);
}

void
vim_buffer_move_to_previous_word_group(VimBuffer *buffer) {
  if (buffer->cursor_byte_offset == 0) {
    if (buffer->cursor_line_index > 0) {
      buffer->cursor_line_index--;
      buffer->cursor_byte_offset = vim_buffer_current_line(buffer)->byte_length;
    } else
      return;
  }

  VimString *line = vim_buffer_current_line(buffer);

  int byte_offset = buffer->cursor_byte_offset;

  if (byte_offset > line->byte_length)
    byte_offset = line->byte_length;

  if (byte_offset > 0)
    byte_offset = vim_previous_character_byte_offset(line->bytes, byte_offset);

  for (;;) {
    while (
      byte_offset > 0 &&
      character_matches_text(line->bytes, line->byte_length, byte_offset, " ")
    )
      byte_offset = vim_previous_character_byte_offset(line->bytes, byte_offset);

    while (byte_offset > 0) {
      int previous_byte_offset =
        vim_previous_character_byte_offset(line->bytes, byte_offset);

      if (character_matches_text(
            line->bytes,
            line->byte_length,
            previous_byte_offset,
            " "
          ))
        break;

      byte_offset = previous_byte_offset;
    }

    if (character_matches_text(
          line->bytes,
          line->byte_length,
          byte_offset,
          " "
        ) &&
        buffer->cursor_line_index > 0) {

      buffer->cursor_line_index--;
      line = vim_buffer_current_line(buffer);
      byte_offset = line->byte_length;

      if (byte_offset > 0)
        byte_offset = vim_previous_character_byte_offset(line->bytes, byte_offset);
    } else
      break;
  }

  buffer->cursor_byte_offset = byte_offset;
  vim_buffer_mark_dirty(buffer, VIM_DIRTY_CURSOR);
}

void
vim_buffer_move_to_word_group_end(VimBuffer *buffer) {
  VimString *line = vim_buffer_current_line(buffer);

  int byte_offset = buffer->cursor_byte_offset + vim_character_byte_length(
                                                   line->bytes,
                                                   line->byte_length,
                                                   buffer->cursor_byte_offset
                                                 );

  for (;;) {
    while (byte_offset >= line->byte_length) {
      if (buffer->cursor_line_index + 1 >= buffer->line_count) {
        vim_buffer_move_to_line_end(buffer);
        return;
      }

      buffer->cursor_line_index++;
      line = vim_buffer_current_line(buffer);
      byte_offset = 0;
    }

    while (
      byte_offset < line->byte_length &&
      character_matches_text(line->bytes, line->byte_length, byte_offset, " ")
    )
      byte_offset +=
        vim_character_byte_length(line->bytes, line->byte_length, byte_offset);

    if (byte_offset < line->byte_length)
      break;
  }

  while (byte_offset < line->byte_length) {
    int next_byte_offset =
      byte_offset +
      vim_character_byte_length(line->bytes, line->byte_length, byte_offset);

    if (next_byte_offset >= line->byte_length || character_matches_text(
                                                   line->bytes,
                                                   line->byte_length,
                                                   next_byte_offset,
                                                   " "
                                                 ))
      break;

    byte_offset = next_byte_offset;
  }

  buffer->cursor_byte_offset = byte_offset;
  vim_buffer_mark_dirty(buffer, VIM_DIRTY_CURSOR);
}

void
vim_buffer_move_to_next_paragraph(VimBuffer *buffer) {
  int line_index = buffer->cursor_line_index + 1;

  while (line_index < buffer->line_count &&
         !is_blank_line(&buffer->lines[line_index]))
    line_index++;

  while (line_index < buffer->line_count &&
         is_blank_line(&buffer->lines[line_index]))
    line_index++;

  int target_line_index = line_index;

  if (target_line_index >= buffer->line_count)
    target_line_index = buffer->line_count - 1;

  vim_buffer_go_to_line(buffer, target_line_index, 1);
}

void
vim_buffer_move_to_previous_paragraph(VimBuffer *buffer) {
  int line_index = buffer->cursor_line_index - 1;

  while (line_index > 0 && is_blank_line(&buffer->lines[line_index]))
    line_index--;

  while (line_index > 0 && !is_blank_line(&buffer->lines[line_index - 1]))
    line_index--;

  int target_line_index = line_index;

  if (target_line_index < 0)
    target_line_index = 0;

  vim_buffer_go_to_line(buffer, target_line_index, 1);
}

int
vim_buffer_find_char_forward(
  VimBuffer *buffer,
  const char *character,
  int character_byte_length,
  int stop_before_match
) {

  VimString *line = vim_buffer_current_line(buffer);

  int byte_offset = buffer->cursor_byte_offset + vim_character_byte_length(
                                                   line->bytes,
                                                   line->byte_length,
                                                   buffer->cursor_byte_offset
                                                 );

  while (byte_offset < line->byte_length) {
    int found_byte_length =
      vim_character_byte_length(line->bytes, line->byte_length, byte_offset);

    if (found_byte_length == character_byte_length &&
        memcmp(
          line->bytes + byte_offset,
          character,
          (size_t)character_byte_length
        ) == 0) {
      if (stop_before_match)
        buffer->cursor_byte_offset =
          vim_previous_character_byte_offset(line->bytes, byte_offset);
      else
        buffer->cursor_byte_offset = byte_offset;

      vim_buffer_mark_dirty(buffer, VIM_DIRTY_CURSOR);

      return 1;
    }

    byte_offset += found_byte_length;
  }

  return 0;
}

int
vim_buffer_find_char_backward(
  VimBuffer *buffer,
  const char *character,
  int character_byte_length,
  int stop_after_match
) {

  VimString *line = vim_buffer_current_line(buffer);

  int byte_offset = buffer->cursor_byte_offset;

  if (byte_offset <= 0)
    return 0;

  byte_offset = vim_previous_character_byte_offset(line->bytes, byte_offset);

  while (byte_offset >= 0) {
    int found_byte_length =
      vim_character_byte_length(line->bytes, line->byte_length, byte_offset);

    if (found_byte_length == character_byte_length &&
        memcmp(
          line->bytes + byte_offset,
          character,
          (size_t)character_byte_length
        ) == 0) {

      buffer->cursor_byte_offset = byte_offset;
      if (stop_after_match)
        buffer->cursor_byte_offset = byte_offset + found_byte_length;

      if (buffer->cursor_byte_offset > line->byte_length)
        buffer->cursor_byte_offset = line->byte_length;

      vim_buffer_mark_dirty(buffer, VIM_DIRTY_CURSOR);

      return 1;
    }

    if (byte_offset == 0)
      break;

    byte_offset = vim_previous_character_byte_offset(line->bytes, byte_offset);
  }

  return 0;
}

int
vim_buffer_find_matching_pair(VimBuffer *buffer) {
  static const char *opens = "([{";
  static const char *closes = ")]}";

  VimString *line = vim_buffer_current_line(buffer);

  if (vim_character_byte_length(
        line->bytes,
        line->byte_length,
        buffer->cursor_byte_offset
      ) != 1)
    return 0;

  char character = 0;
  if (buffer->cursor_byte_offset < line->byte_length)
    character = line->bytes[buffer->cursor_byte_offset];

  const char *open_position = strchr(opens, character);
  const char *close_position = strchr(closes, character);

  int forward;
  char target_character;

  if (open_position && character) {
    forward = 1;
    target_character = closes[open_position - opens];
  } else if (close_position && character) {
    forward = 0;
    target_character = opens[close_position - closes];
  } else
    return 0;

  int depth = 0;
  int line_index = buffer->cursor_line_index;
  int byte_offset = buffer->cursor_byte_offset;

  for (;;) {
    const char *line_bytes = buffer->lines[line_index].bytes;
    int line_byte_length = buffer->lines[line_index].byte_length;

    if (forward) {
      byte_offset +=
        vim_character_byte_length(line_bytes, line_byte_length, byte_offset);

      while (byte_offset >= line_byte_length) {
        line_index++;

        if (line_index >= buffer->line_count)
          return 0;

        line_bytes = buffer->lines[line_index].bytes;
        line_byte_length = buffer->lines[line_index].byte_length;
        byte_offset = 0;

        if (line_byte_length > 0)
          break;
      }
    } else {
      if (byte_offset == 0) {
        line_index--;

        if (line_index < 0)
          return 0;

        line_bytes = buffer->lines[line_index].bytes;
        line_byte_length = buffer->lines[line_index].byte_length;
        byte_offset = line_byte_length;
      }

      byte_offset = vim_previous_character_byte_offset(line_bytes, byte_offset);
    }

    char scanned_character = 0;
    if (byte_offset >= 0 && byte_offset < line_byte_length)
      scanned_character = line_bytes[byte_offset];

    if (scanned_character == character)
      depth++;

    else if (scanned_character == target_character) {
      if (depth == 0) {
        buffer->cursor_line_index = line_index;
        buffer->cursor_byte_offset = byte_offset;
        vim_buffer_mark_dirty(buffer, VIM_DIRTY_CURSOR);
        return 1;
      }

      depth--;
    }
  }
}
