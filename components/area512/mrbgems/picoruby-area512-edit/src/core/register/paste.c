#include "core/register/paste.h"

void
paste_stored_text(Vim *vim, int paste_after_cursor) {
  if (vim->paste.text.byte_length == 0)
    return;

  if (!vim->paste.is_line) {
    if (paste_after_cursor)
      vim_buffer_insert_after_cursor(
        BUFFER,
        vim->paste.text.bytes,
        vim->paste.text.byte_length
      );

    else
      vim_buffer_insert_before_cursor(
        BUFFER,
        vim->paste.text.bytes,
        vim->paste.text.byte_length
      );

  } else {
    if (paste_after_cursor)
      vim_buffer_insert_lines_below(
        BUFFER,
        vim->paste.text.bytes,
        vim->paste.text.byte_length
      );

    else
      vim_buffer_insert_lines_above(
        BUFFER,
        vim->paste.text.bytes,
        vim->paste.text.byte_length
      );
  }
}
