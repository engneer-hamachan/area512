#include "core/buffer/buffer.h"
#include "core/text/utf8.h"

void
vim_buffer_start_selection(VimBuffer *buffer, VimSelectionMode mode) {
  buffer->selection_mode = mode;
  buffer->selection_start_byte_offset = buffer->cursor_byte_offset;
  buffer->selection_start_line_index = buffer->cursor_line_index;
}

void
vim_buffer_clear_selection(VimBuffer *buffer) {
  buffer->selection_mode = VIM_SELECTION_NONE;
  buffer->selection_start_byte_offset = 0;
  buffer->selection_start_line_index = 0;
}

int
vim_buffer_has_selection(VimBuffer *buffer) {
  return buffer->selection_mode != VIM_SELECTION_NONE;
}

int
vim_buffer_selection_range(
  VimBuffer *buffer,
  int *start_line_index,
  int *start_byte_offset,
  int *end_line_index,
  int *end_byte_offset
) {

  if (!vim_buffer_has_selection(buffer))
    return 0;

  int selection_start_line_index = buffer->selection_start_line_index;
  int selection_start_byte_offset = buffer->selection_start_byte_offset;
  int selection_end_line_index = buffer->cursor_line_index;
  int selection_end_byte_offset = buffer->cursor_byte_offset;

  // Cursor can move before the selection anchor, so swap to keep callers
  // free of direction handling: output is always start <= end in text order.
  if (selection_start_line_index > selection_end_line_index ||
      (selection_start_line_index == selection_end_line_index &&
       selection_start_byte_offset > selection_end_byte_offset)) {

    *start_line_index = selection_end_line_index;
    *start_byte_offset = selection_end_byte_offset;
    *end_line_index = selection_start_line_index;
    *end_byte_offset = selection_start_byte_offset;
  } else {
    *start_line_index = selection_start_line_index;
    *start_byte_offset = selection_start_byte_offset;
    *end_line_index = selection_end_line_index;
    *end_byte_offset = selection_end_byte_offset;
  }

  return 1;
}

int
vim_buffer_copy_selected_text(VimBuffer *buffer, VimString *yanked_text) {
  int start_line_index, start_byte_offset, end_line_index, end_byte_offset;

  if (!vim_buffer_selection_range(
        buffer,
        &start_line_index,
        &start_byte_offset,
        &end_line_index,
        &end_byte_offset
      ))
    return 0;

  vim_string_clear(yanked_text);

  if (buffer->selection_mode == VIM_SELECTION_LINE) {
    for (int i = start_line_index; i <= end_line_index; i++) {
      if (i != start_line_index)
        vim_string_append_byte(yanked_text, '\n');

      vim_string_append(
        yanked_text,
        buffer->lines[i].bytes,
        buffer->lines[i].byte_length
      );
    }

  } else if (start_line_index == end_line_index) {
    VimString *line = &buffer->lines[start_line_index];

    vim_string_append_slice(
      yanked_text,
      line->bytes,
      line->byte_length,
      start_byte_offset,
      end_byte_offset + 1
    );

  } else {
    vim_string_append_slice(
      yanked_text,
      buffer->lines[start_line_index].bytes,
      buffer->lines[start_line_index].byte_length,
      start_byte_offset,
      buffer->lines[start_line_index].byte_length
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
      buffer->lines[end_line_index].bytes,
      buffer->lines[end_line_index].byte_length,
      0,
      end_byte_offset + 1
    );
  }

  return 1;
}

int
vim_buffer_delete_selected_text(VimBuffer *buffer, VimString *yanked_text) {
  int start_line_index, start_byte_offset, end_line_index, end_byte_offset;

  if (!vim_buffer_selection_range(
        buffer,
        &start_line_index,
        &start_byte_offset,
        &end_line_index,
        &end_byte_offset
      ))
    return 0;

  if (yanked_text) {
    vim_string_clear(yanked_text);
    vim_buffer_copy_selected_text(buffer, yanked_text);
  }

  if (buffer->selection_mode == VIM_SELECTION_LINE) {
    for (int i = 0; i < (end_line_index - start_line_index + 1); i++)
      vim_buffer_delete_line_at(buffer, start_line_index);

    buffer->cursor_line_index = start_line_index;

    if (buffer->cursor_line_index >= buffer->line_count)
      buffer->cursor_line_index = buffer->line_count - 1;

    buffer->cursor_byte_offset = 0;

    vim_buffer_mark_dirty(buffer, VIM_DIRTY_STRUCTURE);

  } else if (start_line_index == end_line_index) {
    VimString *line = &buffer->lines[start_line_index];
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
    int byte_length = buffer->lines[start_line_index].byte_length;

    if (byte_length > 0 && buffer->cursor_byte_offset >= byte_length)
      buffer->cursor_byte_offset =
        vim_previous_character_byte_offset(
          buffer->lines[start_line_index].bytes,
          byte_length
        );

    if (buffer->cursor_byte_offset < 0)
      buffer->cursor_byte_offset = 0;

    vim_buffer_mark_dirty(buffer, VIM_DIRTY_CONTENT);

  } else {
    VimString line_after_selection;
    vim_string_init(&line_after_selection);

    vim_string_append_slice(
      &line_after_selection,
      buffer->lines[end_line_index].bytes,
      buffer->lines[end_line_index].byte_length,
      end_byte_offset + 1,
      buffer->lines[end_line_index].byte_length
    );

    VimString new_line_content;
    vim_string_init(&new_line_content);

    vim_string_append_slice(
      &new_line_content,
      buffer->lines[start_line_index].bytes,
      buffer->lines[start_line_index].byte_length,
      0,
      start_byte_offset
    );

    vim_string_append(
      &new_line_content,
      line_after_selection.bytes,
      line_after_selection.byte_length
    );

    vim_string_free(&line_after_selection);
    vim_buffer_set_line_built(buffer, start_line_index, &new_line_content);

    for (int j = 0; j < (end_line_index - start_line_index); j++)
      vim_buffer_delete_line_at(buffer, start_line_index + 1);

    buffer->cursor_line_index = start_line_index;
    buffer->cursor_byte_offset = start_byte_offset;
    int byte_length = buffer->lines[start_line_index].byte_length;

    if (byte_length > 0 && buffer->cursor_byte_offset >= byte_length)
      buffer->cursor_byte_offset =
        vim_previous_character_byte_offset(
          buffer->lines[start_line_index].bytes,
          byte_length
        );

    if (buffer->cursor_byte_offset < 0)
      buffer->cursor_byte_offset = 0;

    vim_buffer_mark_dirty(buffer, VIM_DIRTY_STRUCTURE);
  }

  buffer->changed = 1;
  return 1;
}
