#if defined(PICORB_VM_MRUBYC)

#include "area512_hal.h"
#include "core/terminal/terminal.h"

#include <stdio.h>
#include <string.h>

#define AUTOSUGGESTION_TEXT_COLOR_PERCENT 40

static void
draw_terminal_output_row(Filer *filer, int output_row_index) {
  area512_sprite_fill(filer->row, area512_theme_background_color());

  area512_sprite_text(
    filer->row,
    TERMINAL_CONTENT_LEFT_X,
    0,
    filer->terminal->output_lines[output_row_index],
    area512_theme_text_color()
  );

  area512_sprite_push(filer->row, 0, output_row_index * ROW_HEIGHT);
}

static void
draw_prompt_row(Filer *filer) {
  char prompt_text[LINE_MAX];
  int prompt_byte_count;
  int available_input_byte_count;
  int visible_input_byte_offset;
  char visible_input_text[LINE_MAX];
  int visible_input_byte_count;
  char autosuggestion[LINE_MAX];
  int available_autosuggestion_byte_count;
  char visible_autosuggestion_text[LINE_MAX];

  snprintf(
    prompt_text,
    sizeof prompt_text,
    "%s $ ",
    filer->current_directory
  );

  prompt_byte_count = (int)strlen(prompt_text);
  available_input_byte_count =
    filer->terminal->line_byte_capacity - prompt_byte_count;

  if (available_input_byte_count < 1)
    available_input_byte_count = 1;

  visible_input_byte_offset =
    filer->terminal->input_byte_count - available_input_byte_count + 1;

  if (visible_input_byte_offset < 0)
    visible_input_byte_offset = 0;

  fit_string(
    visible_input_text,
    sizeof visible_input_text,
    filer->terminal->input_line + visible_input_byte_offset,
    available_input_byte_count
  );

  visible_input_byte_count = (int)strlen(visible_input_text);

  build_autosuggestion(filer, autosuggestion, sizeof autosuggestion);

  available_autosuggestion_byte_count =
    available_input_byte_count - visible_input_byte_count;

  area512_sprite_fill(filer->row, area512_theme_background_color());

  area512_sprite_text(
    filer->row,
    TERMINAL_CONTENT_LEFT_X,
    0,
    prompt_text,
    area512_theme_emphasis_color()
  );

  area512_sprite_text(
    filer->row,
    TERMINAL_CONTENT_LEFT_X + prompt_byte_count * FILER_CHAR_WIDTH,
    0,
    visible_input_text,
    area512_theme_text_color()
  );

  area512_sprite_text(
    filer->row,
    TERMINAL_CONTENT_LEFT_X +
      (prompt_byte_count + visible_input_byte_count) * FILER_CHAR_WIDTH,
    0,
    "_",
    area512_theme_selected_color()
  );

  if (available_autosuggestion_byte_count > 0) {
    fit_string(
      visible_autosuggestion_text,
      sizeof visible_autosuggestion_text,
      autosuggestion,
      available_autosuggestion_byte_count
    );

    area512_sprite_text(
      filer->row,
      TERMINAL_CONTENT_LEFT_X +
        (prompt_byte_count + visible_input_byte_count) * FILER_CHAR_WIDTH,
      0,
      visible_autosuggestion_text,
      area512_theme_blend_text_color_over_background(
        AUTOSUGGESTION_TEXT_COLOR_PERCENT
      )
    );
  }

  area512_sprite_push(
    filer->row,
    0,
    filer->terminal->output_row_count * ROW_HEIGHT
  );
}

void
draw_terminal(Filer *filer) {
  int output_row_index = 0;

  if (!filer->row)
    return;

  if (filer->full_redraw) {
    area512_gfx_fill_screen(area512_theme_background_color());
    filer->full_redraw = 0;
  }

  while (output_row_index < filer->terminal->output_row_count) {
    draw_terminal_output_row(filer, output_row_index);

    output_row_index++;
  }

  draw_prompt_row(filer);
}

#endif
