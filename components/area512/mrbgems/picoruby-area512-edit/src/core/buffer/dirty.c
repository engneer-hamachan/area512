#include "core/buffer/buffer.h"

void
vim_buffer_mark_dirty(VimBuffer *buffer, VimDirty level) {
  switch (buffer->dirty) {
  case VIM_DIRTY_STRUCTURE:
    break;

  case VIM_DIRTY_CONTENT:
    if (level == VIM_DIRTY_STRUCTURE)
      buffer->dirty = level;

    break;

  case VIM_DIRTY_CURSOR:
    if (level == VIM_DIRTY_CONTENT || level == VIM_DIRTY_STRUCTURE)
      buffer->dirty = level;

    break;

  default:
    buffer->dirty = level;
    break;
  }
}

void
vim_buffer_clear_dirty(VimBuffer *buffer) {
  buffer->dirty = VIM_DIRTY_NONE;
}
