#include "core/complete/complete_popup.h"
#include "area512_hal.h"
#include "core/render/footer.h"
#include "core/text/utf8.h"
#include "port/area512_editor_host.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define COMPLETE_VISIBLE_ROWS 3
#define COMPLETE_TEXT_START_COLUMN 1
#define COMPLETE_WRAP_EXTRA_INDENT 1

#define COMPLETE_PANEL_BACKGROUND 0x402808
#define COMPLETE_NAME_COLOR 0xFFD966
#define COMPLETE_SIGNATURE_COLOR 0xCFA45F
#define COMPLETE_ORIGIN_COLOR 0xF5972D
#define COMPLETE_SELECTED_BACKGROUND 0xF5972D
#define COMPLETE_SELECTED_TEXT_COLOR 0x241604

static int
minimum_complete_value(int first, int second) {
  return first < second ? first : second;
}

static int
usable_complete_columns(Vim *vim) {
  int usable = vim->screen.width - COMPLETE_TEXT_START_COLUMN - 1;

  return usable < 1 ? 1 : usable;
}

static int
measure_complete_class_name_width(const TiSuggestion *suggestion) {
  if (!suggestion->class_name)
    return 0;

  return vim_display_width(
    suggestion->class_name,
    (int)strlen(suggestion->class_name)
  );
}

static int
first_complete_row_limit(Vim *vim, const TiSuggestion *suggestion) {
  int limit = usable_complete_columns(vim);
  int class_name_width = measure_complete_class_name_width(suggestion);

  if (class_name_width > 0)
    limit -= class_name_width + 1;

  return limit < 1 ? 1 : limit;
}

static int
count_complete_suggestion_rows(Vim *vim, const TiSuggestion *suggestion) {
  int detail_width =
    vim_display_width(suggestion->detail, (int)strlen(suggestion->detail));

  return detail_width <= first_complete_row_limit(vim, suggestion) ? 1 : 2;
}

static void
paint_complete_row_background(Vim *vim, int selected) {
  VimCanvas *canvas = vim->active_canvas;
  int width = vim->screen.width;

  canvas->clear_row(canvas->context);

  if (canvas->fill_row_span)
    canvas->fill_row_span(
      canvas->context,
      0,
      width,
      selected ? COMPLETE_SELECTED_BACKGROUND : COMPLETE_PANEL_BACKGROUND
    );
}

static int
draw_complete_detail_part(
  Vim *vim,
  int column,
  const char *detail,
  int start_byte,
  int end_byte,
  int name_byte_length,
  int selected
) {
  VimCanvas *canvas = vim->active_canvas;

  if (start_byte < name_byte_length && start_byte < end_byte) {
    int part_end = minimum_complete_value(end_byte, name_byte_length);

    canvas->draw_row_text(
      canvas->context,
      column,
      detail + start_byte,
      part_end - start_byte,
      selected ? COMPLETE_SELECTED_TEXT_COLOR : COMPLETE_NAME_COLOR,
      0,
      0
    );

    column += vim_display_width(detail + start_byte, part_end - start_byte);
    start_byte = part_end;
  }

  if (start_byte < end_byte) {
    canvas->draw_row_text(
      canvas->context,
      column,
      detail + start_byte,
      end_byte - start_byte,
      selected ? COMPLETE_SELECTED_TEXT_COLOR : COMPLETE_SIGNATURE_COLOR,
      0,
      0
    );

    column += vim_display_width(detail + start_byte, end_byte - start_byte);
  }

  return column;
}

static int
draw_complete_suggestion(
  Vim *vim,
  int screen_row,
  int rows_available,
  const TiSuggestion *suggestion,
  int selected
) {
  VimCanvas *canvas = vim->active_canvas;
  int width = vim->screen.width;
  const char *detail = suggestion->detail;
  int detail_byte_length = (int)strlen(detail);
  int detail_width = vim_display_width(detail, detail_byte_length);

  int name_byte_length = suggestion->contents_length;
  if (name_byte_length > detail_byte_length)
    name_byte_length = detail_byte_length;

  int first_limit = first_complete_row_limit(vim, suggestion);
  int rows_needed = detail_width <= first_limit ? 1 : 2;

  if (rows_needed > rows_available)
    rows_needed = rows_available;

  int first_columns = minimum_complete_value(detail_width, first_limit);
  int first_bytes =
    vim_column_to_byte(detail, detail_byte_length, first_columns);

  paint_complete_row_background(vim, selected);
  draw_complete_detail_part(
    vim,
    COMPLETE_TEXT_START_COLUMN,
    detail,
    0,
    first_bytes,
    name_byte_length,
    selected
  );

  int class_name_width = measure_complete_class_name_width(suggestion);
  if (class_name_width > 0 && class_name_width < width - 1) {
    canvas->draw_row_text(
      canvas->context,
      width - 1 - class_name_width,
      suggestion->class_name,
      (int)strlen(suggestion->class_name),
      selected ? COMPLETE_SELECTED_TEXT_COLOR : COMPLETE_ORIGIN_COLOR,
      0,
      0
    );
  }

  canvas->push_row(canvas->context, screen_row);

  if (rows_needed == 2) {
    int second_limit =
      usable_complete_columns(vim) - COMPLETE_WRAP_EXTRA_INDENT;
    int remaining_bytes = detail_byte_length - first_bytes;
    int second_bytes =
      vim_column_to_byte(detail + first_bytes, remaining_bytes, second_limit);

    paint_complete_row_background(vim, selected);
    draw_complete_detail_part(
      vim,
      COMPLETE_TEXT_START_COLUMN + COMPLETE_WRAP_EXTRA_INDENT,
      detail,
      first_bytes,
      first_bytes + second_bytes,
      name_byte_length,
      selected
    );
    canvas->push_row(canvas->context, screen_row + 1);
  }

  return rows_needed;
}

static void
show_complete_status(
  Vim *vim,
  const TiSuggestionList *suggestions,
  int selected_index,
  int document_scroll
) {
  char position[24];
  int position_length = snprintf(
    position,
    sizeof(position),
    "[%d/%d] ",
    selected_index + 1,
    suggestions->count
  );

  VimString message;
  vim_string_init(&message);
  vim_string_append(&message, position, position_length);

  const char *document = suggestions->items[selected_index].document;
  if (document) {
    int document_length = (int)strlen(document);
    int document_start =
      vim_column_to_byte(document, document_length, document_scroll);

    vim_string_append(
      &message,
      document + document_start,
      document_length - document_start
    );
  }

  show_message(vim, message.bytes, message.byte_length);
  vim_string_free(&message);

  if (vim->screen.footer)
    vim->screen.footer(vim->screen.footer_context, vim->active_canvas);
}

static int
count_complete_rows_between(
  Vim *vim,
  const TiSuggestionList *suggestions,
  int from_index,
  int to_index
) {
  int rows = 0;

  for (int index = from_index; index <= to_index; index++)
    rows += count_complete_suggestion_rows(vim, &suggestions->items[index]);

  return rows;
}

static void
draw_complete_popup(
  Vim *vim,
  const TiSuggestionList *suggestions,
  int selected_index,
  int *window_start,
  int document_scroll
) {
  int available_rows = vim->screen.height - vim->screen.footer_height;
  int content_budget =
    minimum_complete_value(COMPLETE_VISIBLE_ROWS, available_rows);

  if (content_budget <= 0)
    return;

  if (selected_index < *window_start)
    *window_start = selected_index;

  while (count_complete_rows_between(
           vim,
           suggestions,
           *window_start,
           selected_index
         ) > content_budget) {
    (*window_start)++;
  }

  int suggestion_rows[COMPLETE_VISIBLE_ROWS];
  int visible_count = 0;
  int used_rows = 0;

  for (int index = *window_start;
       index < suggestions->count && used_rows < content_budget;
       index++) {

    int rows = count_complete_suggestion_rows(vim, &suggestions->items[index]);

    if (rows > content_budget - used_rows)
      rows = content_budget - used_rows;

    suggestion_rows[visible_count++] = rows;
    used_rows += rows;

    if (visible_count == COMPLETE_VISIBLE_ROWS)
      break;
  }

  int screen_row =
    vim->screen.height - vim->screen.footer_height - used_rows;

  for (int offset = 0; offset < visible_count; offset++) {
    int suggestion_index = *window_start + offset;

    draw_complete_suggestion(
      vim,
      screen_row,
      suggestion_rows[offset],
      &suggestions->items[suggestion_index],
      suggestion_index == selected_index
    );

    screen_row += suggestion_rows[offset];
  }

  show_complete_status(vim, suggestions, selected_index, document_scroll);
}

static int
maximum_complete_document_scroll(
  Vim *vim,
  const TiSuggestionList *suggestions,
  int selected_index
) {
  const char *document = suggestions->items[selected_index].document;
  if (!document)
    return 0;

  int document_width = vim_display_width(document, (int)strlen(document));
  char position[24];
  int position_width = snprintf(
    position,
    sizeof(position),
    "[%d/%d] ",
    selected_index + 1,
    suggestions->count
  );
  int visible_width = vim->screen.width - position_width;

  if (visible_width < 1)
    visible_width = 1;

  int maximum = document_width - visible_width;
  return maximum > 0 ? maximum : 0;
}

static int
read_complete_printable(
  int first_byte,
  char character[4],
  int *character_byte_length
) {
  int byte_length = vim_utf8_byte_length((uint8_t)first_byte);

  if (byte_length < 1)
    byte_length = 1;
  if (byte_length > 4)
    byte_length = 4;

  character[0] = (char)first_byte;

  for (int index = 1; index < byte_length; index++) {
    int continuation_byte = area512_console_getch_block();

    if (continuation_byte < 0)
      return 0;

    character[index] = (char)continuation_byte;
  }

  *character_byte_length = byte_length;
  return 1;
}

static void
remove_complete_prefix(Vim *vim) {
  VimString *line = &BUFFER->lines[BUFFER->cursor_line_index];

  while (BUFFER->cursor_byte_offset > 0) {
    int previous_offset = BUFFER->cursor_byte_offset - 1;
    uint8_t byte = (uint8_t)line->bytes[previous_offset];

    if (!((byte >= 'a' && byte <= 'z') || (byte >= 'A' && byte <= 'Z') ||
          (byte >= '0' && byte <= '9') || byte == '_' || byte == '!' ||
          byte == '?')) {
      break;
    }

    vim_buffer_put_key(BUFFER, VIM_PUT_BACKSPACE);
  }
}

int
show_complete_popup(
  Vim *vim,
  const TiSuggestionList *suggestions,
  int *next_key,
  char next_character[4],
  int *next_character_byte_length
) {
  if (!vim || !vim->active_canvas || !suggestions || suggestions->count <= 0)
    return 0;

  int selected_index = 0;
  int window_start = 0;
  int document_scroll = 0;
  draw_complete_popup(
    vim,
    suggestions,
    selected_index,
    &window_start,
    document_scroll
  );

  for (;;) {
    int key = area512_console_getch_block();

    if (key < 0)
      continue;

    if (key == 14) {
      selected_index = (selected_index + 1) % suggestions->count;
      document_scroll = 0;
      draw_complete_popup(
        vim,
        suggestions,
        selected_index,
        &window_start,
        document_scroll
      );
      continue;
    }

    if (key == 10 || key == 13) {
      const TiSuggestion *suggestion = &suggestions->items[selected_index];
      remove_complete_prefix(vim);
      vim_buffer_put_string(
        BUFFER,
        suggestion->contents,
        suggestion->contents_length
      );
      REDRAW(VIM_REDRAW_ALL);
      return 0;
    }

    if (key == 27) {
      char sequence[2];
      int sequence_length = read_escape_sequence(sequence);

      if (sequence_length >= 2 && sequence[0] == '[') {
        if (sequence[1] == 'A') {
          selected_index--;
          if (selected_index < 0)
            selected_index = suggestions->count - 1;
          document_scroll = 0;
          draw_complete_popup(
            vim,
            suggestions,
            selected_index,
            &window_start,
            document_scroll
          );
          continue;
        }

        if (sequence[1] == 'B') {
          selected_index = (selected_index + 1) % suggestions->count;
          document_scroll = 0;
          draw_complete_popup(
            vim,
            suggestions,
            selected_index,
            &window_start,
            document_scroll
          );
          continue;
        }

        if (sequence[1] == 'C') {
          int maximum = maximum_complete_document_scroll(
            vim,
            suggestions,
            selected_index
          );

          if (document_scroll < maximum)
            document_scroll++;

          draw_complete_popup(
            vim,
            suggestions,
            selected_index,
            &window_start,
            document_scroll
          );
          continue;
        }

        if (sequence[1] == 'D') {
          if (document_scroll > 0)
            document_scroll--;

          draw_complete_popup(
            vim,
            suggestions,
            selected_index,
            &window_start,
            document_scroll
          );
          continue;
        }
      }

      REDRAW(VIM_REDRAW_ALL);
      return 0;
    }

    if (key == 8 || key == 127) {
      *next_key = key;
      *next_character_byte_length = 0;
      REDRAW(VIM_REDRAW_ALL);
      return 1;
    }

    if (key >= 32 && read_complete_printable(
                       key,
                       next_character,
                       next_character_byte_length
                     )) {
      *next_key = key;
      REDRAW(VIM_REDRAW_ALL);
      return 1;
    }
  }
}
