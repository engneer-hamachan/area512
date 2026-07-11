#include "core/buffer/buffer.h"
#include <stdlib.h>
#include <string.h>

void
vim_buffer_init(VimBuffer *buffer) {
  buffer->lines = NULL;
  buffer->line_count = 0;
  buffer->capacity = 0;
  buffer->cursor_byte_offset = 0;
  buffer->cursor_line_index = 0;
  buffer->changed = 0;
  buffer->dirty = VIM_DIRTY_NONE;
  buffer->selection_mode = VIM_SELECTION_NONE;
  buffer->selection_start_byte_offset = 0;
  buffer->selection_start_line_index = 0;
  vim_buffer_clear(buffer);
}

void
vim_buffer_free(VimBuffer *buffer) {
  for (int i = 0; i < buffer->line_count; i++)
    vim_string_free(&buffer->lines[i]);

  if (buffer->lines)
    free(buffer->lines);

  buffer->lines = NULL;
  buffer->line_count = 0;
  buffer->capacity = 0;
}

static int
vim_buffer_grow(VimBuffer *buffer, int needed_capacity) {
  if (needed_capacity <= buffer->capacity)
    return 1;

  int new_capacity = 8;
  if (buffer->capacity)
    new_capacity = buffer->capacity * 2;

  while (new_capacity < needed_capacity)
    new_capacity *= 2;

  VimString *new_lines = (VimString *)
    realloc(buffer->lines, (size_t)new_capacity * sizeof(VimString));

  if (!new_lines)
    return 0;

  buffer->lines = new_lines;
  buffer->capacity = new_capacity;

  return 1;
}

void
vim_buffer_clear(VimBuffer *buffer) {
  for (int i = 0; i < buffer->line_count; i++)
    vim_string_free(&buffer->lines[i]);

  buffer->line_count = 0;

  vim_buffer_grow(buffer, 1);
  vim_string_init(&buffer->lines[0]);

  buffer->line_count = 1;
  buffer->changed = 0;

  vim_buffer_clear_selection(buffer);
  vim_buffer_move_home(buffer);
}

void
vim_buffer_load_text(VimBuffer *buffer, const char *text, int byte_length) {
  vim_buffer_clear(buffer);

  int content_byte_length = byte_length;
  if (content_byte_length > 0 && text[content_byte_length - 1] == '\n')
    content_byte_length--;

  int start = 0, first = 1;
  for (int i = 0; i <= content_byte_length; i++) {
    if (i == content_byte_length || text[i] == '\n') {
      if (first) {
        vim_buffer_set_line(buffer, 0, text + start, i - start);
        first = 0;
      } else
        vim_buffer_insert_line(buffer, buffer->line_count, text + start, i - start);

      start = i + 1;
    }
  }

  buffer->changed = 0;
}

int
vim_buffer_insert_line(
  VimBuffer *buffer,
  int line_index,
  const char *text,
  int byte_length
) {

  if (line_index < 0)
    line_index = 0;

  if (line_index > buffer->line_count)
    line_index = buffer->line_count;

  if (!vim_buffer_grow(buffer, buffer->line_count + 1))
    return 0;

  memmove(
    &buffer->lines[line_index + 1],
    &buffer->lines[line_index],
    (size_t)(buffer->line_count - line_index) * sizeof(VimString)
  );

  vim_string_init(&buffer->lines[line_index]);
  vim_string_append(&buffer->lines[line_index], text, byte_length);

  buffer->line_count++;

  return 1;
}

void
vim_buffer_delete_line_at(VimBuffer *buffer, int line_index) {
  if (line_index < 0 || line_index >= buffer->line_count)
    return;

  vim_string_free(&buffer->lines[line_index]);

  memmove(
    &buffer->lines[line_index],
    &buffer->lines[line_index + 1],
    (size_t)(buffer->line_count - line_index - 1) * sizeof(VimString)
  );

  buffer->line_count--;

  if (buffer->line_count == 0) {
    vim_string_init(&buffer->lines[0]);
    buffer->line_count = 1;
  }
}

int
vim_buffer_set_line(
  VimBuffer *buffer,
  int line_index,
  const char *text,
  int byte_length
) {

  if (line_index < 0 || line_index >= buffer->line_count)
    return 0;

  return vim_string_set(&buffer->lines[line_index], text, byte_length);
}

// Reuses the built string's buffer instead of copying it; built_line is left
// emptied.
void
vim_buffer_set_line_built(
  VimBuffer *buffer,
  int line_index,
  VimString *built_line
) {
  if (line_index < 0 || line_index >= buffer->line_count) {
    vim_string_free(built_line);
    return;
  }

  vim_string_free(&buffer->lines[line_index]);

  buffer->lines[line_index] = *built_line;
  built_line->bytes = NULL;
  built_line->byte_length = 0;
  built_line->capacity = 0;
}
