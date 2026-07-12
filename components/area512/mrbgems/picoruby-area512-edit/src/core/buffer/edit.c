#include "core/buffer/buffer.h"
#include "core/text/utf8.h"
#include <stdlib.h>
#include <string.h>

void
vim_buffer_put_string(VimBuffer *buffer, const char *text, int byte_length) {
  VimString *line = vim_buffer_current_line(buffer);

  if (line->byte_length < buffer->cursor_byte_offset)
    buffer->cursor_byte_offset = line->byte_length;

  buffer->changed = 1;

  VimString new_line_content;

  vim_string_init(&new_line_content);

  vim_string_append_slice(
    &new_line_content,
    line->bytes,
    line->byte_length,
    0,
    buffer->cursor_byte_offset
  );

  vim_string_append(&new_line_content, text, byte_length);

  vim_string_append_slice(
    &new_line_content,
    line->bytes,
    line->byte_length,
    buffer->cursor_byte_offset,
    line->byte_length
  );

  vim_buffer_set_line_built(
    buffer,
    buffer->cursor_line_index,
    &new_line_content
  );

  buffer->cursor_byte_offset += byte_length;

  vim_buffer_mark_dirty(buffer, VIM_DIRTY_CONTENT);
}

void
vim_buffer_put_key(VimBuffer *buffer, VimPutKey key) {
  switch (key) {
  case VIM_PUT_TAB:
    buffer->changed = 1;
    vim_buffer_put_string(buffer, " ", 1);
    vim_buffer_put_string(buffer, " ", 1);

    break;

  case VIM_PUT_ENTER: {
    buffer->changed = 1;
    VimString *line = vim_buffer_current_line(buffer);

    int cursor_byte_offset = buffer->cursor_byte_offset;
    if (cursor_byte_offset > line->byte_length)
      cursor_byte_offset = line->byte_length;

    VimString line_before_cursor;

    vim_string_init(&line_before_cursor);

    vim_string_append_slice(
      &line_before_cursor,
      line->bytes,
      line->byte_length,
      0,
      cursor_byte_offset
    );

    VimString line_after_cursor;
    vim_string_init(&line_after_cursor);

    vim_string_append_slice(
      &line_after_cursor,
      line->bytes,
      line->byte_length,
      cursor_byte_offset,
      line->byte_length
    );

    vim_buffer_set_line_built(
      buffer,
      buffer->cursor_line_index,
      &line_before_cursor
    );

    vim_buffer_insert_line(
      buffer,
      buffer->cursor_line_index + 1,
      line_after_cursor.bytes,
      line_after_cursor.byte_length
    );

    vim_string_free(&line_after_cursor);
    vim_buffer_mark_dirty(buffer, VIM_DIRTY_STRUCTURE);
    vim_buffer_move_to_line_head(buffer);
    vim_buffer_move_down(buffer);

    break;
  }
  case VIM_PUT_BACKSPACE: {
    buffer->changed = 1;

    if (buffer->cursor_byte_offset > 0) {
      VimString *line = vim_buffer_current_line(buffer);

      int previous_byte_offset = vim_previous_character_byte_offset(
        line->bytes,
        buffer->cursor_byte_offset
      );

      VimString new_line_content;
      vim_string_init(&new_line_content);

      vim_string_append_slice(
        &new_line_content,
        line->bytes,
        line->byte_length,
        0,
        previous_byte_offset
      );

      vim_string_append_slice(
        &new_line_content,
        line->bytes,
        line->byte_length,
        buffer->cursor_byte_offset,
        line->byte_length
      );

      vim_buffer_set_line_built(
        buffer,
        buffer->cursor_line_index,
        &new_line_content
      );

      buffer->cursor_byte_offset = previous_byte_offset;

      vim_buffer_mark_dirty(buffer, VIM_DIRTY_CONTENT);

    } else if (buffer->cursor_line_index > 0) {
      int previous_byte_length =
        buffer->lines[buffer->cursor_line_index - 1].byte_length;
      VimString *current_line = vim_buffer_current_line(buffer);

      vim_string_append(
        &buffer->lines[buffer->cursor_line_index - 1],
        current_line->bytes,
        current_line->byte_length
      );

      vim_buffer_delete_line_at(buffer, buffer->cursor_line_index);

      buffer->cursor_byte_offset = previous_byte_length;

      vim_buffer_mark_dirty(buffer, VIM_DIRTY_STRUCTURE);
      vim_buffer_move_up(buffer);
    }

    break;
  }
  case VIM_PUT_DOWN:
    vim_buffer_move_down(buffer);
    break;

  case VIM_PUT_UP:
    vim_buffer_move_up(buffer);
    break;

  case VIM_PUT_RIGHT:
    vim_buffer_move_right(buffer);
    break;

  case VIM_PUT_LEFT:
    vim_buffer_move_left(buffer);
    break;

  case VIM_PUT_HOME:
    vim_buffer_move_home(buffer);
    break;
  }
}

void
vim_buffer_delete_char(VimBuffer *buffer) {
  VimString *line = vim_buffer_current_line(buffer);

  int character_byte_length = vim_character_byte_length(
    line->bytes,
    line->byte_length,
    buffer->cursor_byte_offset
  );

  if (character_byte_length == 0)
    return;

  VimString new_line_content;

  vim_string_init(&new_line_content);

  vim_string_append_slice(
    &new_line_content,
    line->bytes,
    line->byte_length,
    0,
    buffer->cursor_byte_offset
  );

  vim_string_append_slice(
    &new_line_content,
    line->bytes,
    line->byte_length,
    buffer->cursor_byte_offset + character_byte_length,
    line->byte_length
  );

  vim_buffer_set_line_built(
    buffer,
    buffer->cursor_line_index,
    &new_line_content
  );

  buffer->changed = 1;

  vim_buffer_mark_dirty(buffer, VIM_DIRTY_CONTENT);
}

int
vim_buffer_delete_line(VimBuffer *buffer, VimString *yanked_text) {
  if (yanked_text) {
    vim_string_clear(yanked_text);

    vim_string_append(
      yanked_text,
      buffer->lines[buffer->cursor_line_index].bytes,
      buffer->lines[buffer->cursor_line_index].byte_length
    );
  }

  vim_buffer_delete_line_at(buffer, buffer->cursor_line_index);

  if (buffer->cursor_line_index >= buffer->line_count)
    buffer->cursor_line_index = buffer->line_count - 1;

  buffer->cursor_byte_offset = 0;
  buffer->changed = 1;

  vim_buffer_mark_dirty(buffer, VIM_DIRTY_STRUCTURE);

  return 1;
}

void
vim_buffer_replace_char(
  VimBuffer *buffer,
  const char *character,
  int byte_length
) {

  VimString *line = vim_buffer_current_line(buffer);

  if (buffer->cursor_byte_offset >= line->byte_length)
    return;

  int character_byte_length = vim_character_byte_length(
    line->bytes,
    line->byte_length,
    buffer->cursor_byte_offset
  );

  VimString new_line_content;
  vim_string_init(&new_line_content);

  vim_string_append_slice(
    &new_line_content,
    line->bytes,
    line->byte_length,
    0,
    buffer->cursor_byte_offset
  );

  vim_string_append(&new_line_content, character, byte_length);

  vim_string_append_slice(
    &new_line_content,
    line->bytes,
    line->byte_length,
    buffer->cursor_byte_offset + character_byte_length,
    line->byte_length
  );

  vim_buffer_set_line_built(
    buffer,
    buffer->cursor_line_index,
    &new_line_content
  );

  buffer->changed = 1;

  vim_buffer_mark_dirty(buffer, VIM_DIRTY_CONTENT);
}

int
vim_buffer_delete_range(
  VimBuffer *buffer,
  int start_line_index,
  int start_byte_offset,
  int end_line_index,
  int end_byte_offset,
  VimString *yanked_text
) {

  if (yanked_text)
    vim_string_clear(yanked_text);

  if (start_line_index > end_line_index ||
      (start_line_index == end_line_index && start_byte_offset > end_byte_offset
      )) {

    int temporary_line_index = start_line_index;
    int temporary_byte_offset = start_byte_offset;
    start_line_index = end_line_index;
    start_byte_offset = end_byte_offset;
    end_line_index = temporary_line_index;
    end_byte_offset = temporary_byte_offset;
  }

  if (start_line_index < 0)
    start_line_index = 0;

  if (end_line_index >= buffer->line_count)
    end_line_index = buffer->line_count - 1;

  if (start_line_index > end_line_index ||
      start_line_index >= buffer->line_count)
    return 0;

  if (start_byte_offset < 0)
    start_byte_offset = 0;

  if (end_byte_offset < 0)
    end_byte_offset = 0;

  if (start_line_index == end_line_index) {
    VimString *line = &buffer->lines[start_line_index];

    if (end_byte_offset >= line->byte_length)
      end_byte_offset = line->byte_length - 1;

    if (start_byte_offset >= line->byte_length ||
        end_byte_offset < start_byte_offset)
      return 0;

    if (yanked_text)
      vim_string_append_slice(
        yanked_text,
        line->bytes,
        line->byte_length,
        start_byte_offset,
        end_byte_offset + 1
      );

    VimString new_line_content;
    vim_string_init(&new_line_content);

    vim_string_append_slice(
      &new_line_content,
      line->bytes,
      line->byte_length,
      0,
      start_byte_offset
    );

    vim_string_append_slice(
      &new_line_content,
      line->bytes,
      line->byte_length,
      end_byte_offset + 1,
      line->byte_length
    );

    vim_buffer_set_line_built(buffer, start_line_index, &new_line_content);

    buffer->cursor_line_index = start_line_index;
    buffer->cursor_byte_offset = start_byte_offset;

    vim_buffer_clamp_cursor(buffer);
    vim_buffer_mark_dirty(buffer, VIM_DIRTY_CONTENT);
  } else {
    VimString *first = &buffer->lines[start_line_index];
    VimString *last = &buffer->lines[end_line_index];

    if (end_byte_offset >= last->byte_length)
      end_byte_offset = last->byte_length - 1;

    if (yanked_text) {
      vim_string_append_slice(
        yanked_text,
        first->bytes,
        first->byte_length,
        start_byte_offset,
        first->byte_length
      );

      for (int i = start_line_index + 1; i < end_line_index; i++) {
        vim_string_append_byte(yanked_text, '\n');
        vim_string_append(
          yanked_text,
          buffer->lines[i].bytes,
          buffer->lines[i].byte_length
        );
      }

      vim_string_append_byte(yanked_text, '\n');
      vim_string_append_slice(
        yanked_text,
        last->bytes,
        last->byte_length,
        0,
        end_byte_offset + 1
      );
    }

    VimString new_line_content;
    vim_string_init(&new_line_content);

    vim_string_append_slice(
      &new_line_content,
      first->bytes,
      first->byte_length,
      0,
      start_byte_offset
    );

    vim_string_append_slice(
      &new_line_content,
      last->bytes,
      last->byte_length,
      end_byte_offset + 1,
      last->byte_length
    );

    vim_buffer_set_line_built(buffer, start_line_index, &new_line_content);

    for (int j = start_line_index + 1; j <= end_line_index; j++)
      vim_buffer_delete_line_at(buffer, start_line_index + 1);

    buffer->cursor_line_index = start_line_index;
    buffer->cursor_byte_offset = start_byte_offset;
    vim_buffer_clamp_cursor(buffer);
    vim_buffer_mark_dirty(buffer, VIM_DIRTY_STRUCTURE);
  }

  buffer->changed = 1;

  return 1;
}

int
vim_buffer_delete_to_eol(VimBuffer *buffer, VimString *yanked_text) {
  VimString *line = vim_buffer_current_line(buffer);
  if (buffer->cursor_byte_offset >= line->byte_length)
    return 0;

  return vim_buffer_delete_range(
    buffer,
    buffer->cursor_line_index,
    buffer->cursor_byte_offset,
    buffer->cursor_line_index,
    line->byte_length - 1,
    yanked_text
  );
}

int
vim_buffer_join_next_line(VimBuffer *buffer) {
  if (buffer->cursor_line_index + 1 >= buffer->line_count)
    return 0;

  VimString *line = vim_buffer_current_line(buffer);
  int original_byte_length = line->byte_length;
  VimString *next_line = &buffer->lines[buffer->cursor_line_index + 1];

  int strip_start_index = 0;
  while (strip_start_index < next_line->byte_length &&
         (next_line->bytes[strip_start_index] == ' ' ||
          next_line->bytes[strip_start_index] == '\t'))
    strip_start_index++;

  VimString new_line_content;
  vim_string_init(&new_line_content);

  vim_string_append(&new_line_content, line->bytes, line->byte_length);
  vim_string_append_byte(&new_line_content, ' ');

  vim_string_append_slice(
    &new_line_content,
    next_line->bytes,
    next_line->byte_length,
    strip_start_index,
    next_line->byte_length
  );

  vim_buffer_delete_line_at(buffer, buffer->cursor_line_index + 1);
  vim_buffer_set_line_built(
    buffer,
    buffer->cursor_line_index,
    &new_line_content
  );

  buffer->cursor_byte_offset = original_byte_length;
  buffer->changed = 1;

  vim_buffer_mark_dirty(buffer, VIM_DIRTY_STRUCTURE);

  return 1;
}

void
vim_buffer_indent_line_at(
  VimBuffer *buffer,
  int line_index,
  const char *indent,
  int indent_byte_length
) {
  VimString *line = &buffer->lines[line_index];

  VimString new_line_content;
  vim_string_init(&new_line_content);

  vim_string_append(&new_line_content, indent, indent_byte_length);
  vim_string_append(&new_line_content, line->bytes, line->byte_length);
  vim_buffer_set_line_built(buffer, line_index, &new_line_content);

  if (line_index == buffer->cursor_line_index)
    buffer->cursor_byte_offset += indent_byte_length;

  buffer->changed = 1;

  vim_buffer_mark_dirty(buffer, VIM_DIRTY_CONTENT);
}

void
vim_buffer_indent_line(
  VimBuffer *buffer,
  const char *indent,
  int indent_byte_length
) {

  vim_buffer_indent_line_at(
    buffer,
    buffer->cursor_line_index,
    indent,
    indent_byte_length
  );
}

void
vim_buffer_outdent_line_at(
  VimBuffer *buffer,
  int line_index,
  const char *indent,
  int indent_byte_length
) {

  VimString *line = &buffer->lines[line_index];

  int remove_byte_length = 0;

  if (line->byte_length >= indent_byte_length &&
      memcmp(line->bytes, indent, (size_t)indent_byte_length) == 0)

    remove_byte_length = indent_byte_length;

  else if (line->byte_length > 0 && line->bytes[0] == ' ')
    remove_byte_length = 1;

  if (remove_byte_length == 0)
    return;

  VimString new_line_content;
  vim_string_init(&new_line_content);

  vim_string_append_slice(
    &new_line_content,
    line->bytes,
    line->byte_length,
    remove_byte_length,
    line->byte_length
  );

  vim_buffer_set_line_built(buffer, line_index, &new_line_content);

  if (line_index == buffer->cursor_line_index) {
    buffer->cursor_byte_offset -= remove_byte_length;

    if (buffer->cursor_byte_offset < 0)
      buffer->cursor_byte_offset = 0;
  }

  buffer->changed = 1;
  vim_buffer_mark_dirty(buffer, VIM_DIRTY_CONTENT);
}

void
vim_buffer_outdent_line(
  VimBuffer *buffer,
  const char *indent,
  int indent_byte_length
) {

  vim_buffer_outdent_line_at(
    buffer,
    buffer->cursor_line_index,
    indent,
    indent_byte_length
  );
}

void
vim_buffer_toggle_case(VimBuffer *buffer) {
  VimString *line = vim_buffer_current_line(buffer);

  if (buffer->cursor_byte_offset >= line->byte_length)
    return;

  if (vim_character_byte_length(
        line->bytes,
        line->byte_length,
        buffer->cursor_byte_offset
      ) != 1)
    return;

  unsigned char original =
    (unsigned char)line->bytes[buffer->cursor_byte_offset];

  char toggled;
  if (original >= 'A' && original <= 'Z')
    toggled = (char)(original + 32);
  else if (original >= 'a' && original <= 'z')
    toggled = (char)(original - 32);
  else
    return;

  vim_buffer_replace_char(buffer, &toggled, 1);
  vim_buffer_move_right(buffer);
}

typedef struct {
  const char *text;
  int byte_length;
} TextSegment;

static int
split_text_by_newlines(
  const char *text,
  int byte_length,
  TextSegment **result_segments
) {

  int estimated_segment_count = 1;
  for (int i = 0; i < byte_length; i++)
    if (text[i] == '\n')
      estimated_segment_count++;

  TextSegment *segments = (TextSegment *)malloc(
    (size_t)estimated_segment_count * sizeof(TextSegment)
  );

  if (!segments) {
    *result_segments = NULL;
    return 0;
  }

  int segment_count = 0, segment_start = 0;

  for (int i = 0; i < byte_length; i++) {
    if (text[i] == '\n') {
      segments[segment_count].text = text + segment_start;
      segments[segment_count].byte_length = i - segment_start;
      segment_count++;
      segment_start = i + 1;
    }
  }

  segments[segment_count].text = text + segment_start;
  segments[segment_count].byte_length = byte_length - segment_start;
  segment_count++;

  while (segment_count > 0 && segments[segment_count - 1].byte_length == 0)
    segment_count--;

  if (segment_count == 0) {
    free(segments);
    *result_segments = NULL;
    return 0;
  }

  *result_segments = segments;

  return segment_count;
}

void
vim_buffer_insert_after_cursor(
  VimBuffer *buffer,
  const char *text,
  int byte_length
) {

  VimString *line = vim_buffer_current_line(buffer);
  int insert_byte_offset =
    buffer->cursor_byte_offset + vim_character_byte_length(
                                   line->bytes,
                                   line->byte_length,
                                   buffer->cursor_byte_offset
                                 );

  if (insert_byte_offset > line->byte_length)
    insert_byte_offset = line->byte_length;

  TextSegment *segments;
  int segment_count = split_text_by_newlines(text, byte_length, &segments);

  if (segment_count <= 1) {
    const char *segment_text = "";

    int segment_byte_length = 0;
    if (segment_count == 1) {
      segment_text = segments[0].text;
      segment_byte_length = segments[0].byte_length;
    }

    VimString new_line_content;
    vim_string_init(&new_line_content);

    vim_string_append_slice(
      &new_line_content,
      line->bytes,
      line->byte_length,
      0,
      insert_byte_offset
    );

    vim_string_append(&new_line_content, segment_text, segment_byte_length);

    vim_string_append_slice(
      &new_line_content,
      line->bytes,
      line->byte_length,
      insert_byte_offset,
      line->byte_length
    );

    vim_buffer_set_line_built(
      buffer,
      buffer->cursor_line_index,
      &new_line_content
    );

    int end_insert_byte_offset = insert_byte_offset + segment_byte_length;
    VimString *updated_line = vim_buffer_current_line(buffer);
    buffer->cursor_byte_offset = 0;

    if (end_insert_byte_offset > 0)
      buffer->cursor_byte_offset = vim_previous_character_byte_offset(
        updated_line->bytes,
        end_insert_byte_offset
      );

    if (buffer->cursor_byte_offset < 0)
      buffer->cursor_byte_offset = 0;

    vim_buffer_mark_dirty(buffer, VIM_DIRTY_CONTENT);
  } else {
    VimString line_after_cursor;
    vim_string_init(&line_after_cursor);

    vim_string_append_slice(
      &line_after_cursor,
      line->bytes,
      line->byte_length,
      insert_byte_offset,
      line->byte_length
    );

    VimString new_line_content;
    vim_string_init(&new_line_content);

    vim_string_append_slice(
      &new_line_content,
      line->bytes,
      line->byte_length,
      0,
      insert_byte_offset
    );

    vim_string_append(
      &new_line_content,
      segments[0].text,
      segments[0].byte_length
    );
    vim_buffer_set_line_built(
      buffer,
      buffer->cursor_line_index,
      &new_line_content
    );

    for (int i = 1; i < segment_count; i++)
      vim_buffer_insert_line(
        buffer,
        buffer->cursor_line_index + i,
        segments[i].text,
        segments[i].byte_length
      );

    int last_index = buffer->cursor_line_index + segment_count - 1;

    vim_string_append(
      &buffer->lines[last_index],
      line_after_cursor.bytes,
      line_after_cursor.byte_length
    );

    vim_string_free(&line_after_cursor);

    buffer->cursor_line_index = last_index;
    int last_segment_byte_length = segments[segment_count - 1].byte_length;
    buffer->cursor_byte_offset = 0;

    if (last_segment_byte_length > 0)
      buffer->cursor_byte_offset = vim_previous_character_byte_offset(
        buffer->lines[last_index].bytes,
        last_segment_byte_length
      );

    vim_buffer_mark_dirty(buffer, VIM_DIRTY_STRUCTURE);
  }

  if (segments)
    free(segments);

  buffer->changed = 1;
}

void
vim_buffer_insert_before_cursor(
  VimBuffer *buffer,
  const char *text,
  int byte_length
) {

  VimString *line = vim_buffer_current_line(buffer);
  int insert_byte_offset = buffer->cursor_byte_offset;

  TextSegment *segments;
  int segment_count = split_text_by_newlines(text, byte_length, &segments);

  if (segment_count <= 1) {
    const char *segment_text = "";
    int segment_byte_length = 0;

    if (segment_count == 1) {
      segment_text = segments[0].text;
      segment_byte_length = segments[0].byte_length;
    }

    VimString new_line_content;
    vim_string_init(&new_line_content);

    vim_string_append_slice(
      &new_line_content,
      line->bytes,
      line->byte_length,
      0,
      insert_byte_offset
    );

    vim_string_append(&new_line_content, segment_text, segment_byte_length);

    vim_string_append_slice(
      &new_line_content,
      line->bytes,
      line->byte_length,
      insert_byte_offset,
      line->byte_length
    );

    vim_buffer_set_line_built(
      buffer,
      buffer->cursor_line_index,
      &new_line_content
    );

    buffer->cursor_byte_offset = insert_byte_offset + segment_byte_length;
    VimString *updated_line = vim_buffer_current_line(buffer);

    if (buffer->cursor_byte_offset > 0)
      buffer->cursor_byte_offset = vim_previous_character_byte_offset(
        updated_line->bytes,
        buffer->cursor_byte_offset
      );

    vim_buffer_mark_dirty(buffer, VIM_DIRTY_CONTENT);

  } else {
    VimString line_after_cursor;
    vim_string_init(&line_after_cursor);

    vim_string_append_slice(
      &line_after_cursor,
      line->bytes,
      line->byte_length,
      insert_byte_offset,
      line->byte_length
    );

    VimString new_line_content;
    vim_string_init(&new_line_content);

    vim_string_append_slice(
      &new_line_content,
      line->bytes,
      line->byte_length,
      0,
      insert_byte_offset
    );

    vim_string_append(
      &new_line_content,
      segments[0].text,
      segments[0].byte_length
    );
    vim_buffer_set_line_built(
      buffer,
      buffer->cursor_line_index,
      &new_line_content
    );

    for (int i = 1; i < segment_count; i++)
      vim_buffer_insert_line(
        buffer,
        buffer->cursor_line_index + i,
        segments[i].text,
        segments[i].byte_length
      );

    int last_index = buffer->cursor_line_index + segment_count - 1;

    vim_string_append(
      &buffer->lines[last_index],
      line_after_cursor.bytes,
      line_after_cursor.byte_length
    );

    vim_string_free(&line_after_cursor);

    buffer->cursor_line_index = last_index;

    int last_segment_byte_length = segments[segment_count - 1].byte_length;
    buffer->cursor_byte_offset = last_segment_byte_length;

    if (buffer->cursor_byte_offset > 0)
      buffer->cursor_byte_offset = vim_previous_character_byte_offset(
        buffer->lines[last_index].bytes,
        buffer->cursor_byte_offset
      );

    vim_buffer_mark_dirty(buffer, VIM_DIRTY_STRUCTURE);
  }

  if (segments)
    free(segments);

  buffer->changed = 1;
}

void
vim_buffer_insert_lines_below(
  VimBuffer *buffer,
  const char *text,
  int byte_length
) {

  TextSegment *segments;
  int segment_count = split_text_by_newlines(text, byte_length, &segments);

  int insert_line_index = buffer->cursor_line_index + 1;

  for (int i = 0; i < segment_count; i++)
    vim_buffer_insert_line(
      buffer,
      insert_line_index + i,
      segments[i].text,
      segments[i].byte_length
    );

  if (segments)
    free(segments);

  buffer->cursor_line_index = insert_line_index;

  if (buffer->cursor_line_index >= buffer->line_count)
    buffer->cursor_line_index = buffer->line_count - 1;

  buffer->cursor_byte_offset = 0;

  vim_buffer_mark_dirty(buffer, VIM_DIRTY_STRUCTURE);
  buffer->changed = 1;
}

void
vim_buffer_insert_lines_above(
  VimBuffer *buffer,
  const char *text,
  int byte_length
) {
  TextSegment *segments;
  int segment_count = split_text_by_newlines(text, byte_length, &segments);

  int insert_line_index = buffer->cursor_line_index;

  for (int i = 0; i < segment_count; i++)
    vim_buffer_insert_line(
      buffer,
      insert_line_index + i,
      segments[i].text,
      segments[i].byte_length
    );

  if (segments)
    free(segments);

  buffer->cursor_line_index = insert_line_index;
  buffer->cursor_byte_offset = 0;

  vim_buffer_mark_dirty(buffer, VIM_DIRTY_STRUCTURE);
  buffer->changed = 1;
}
