#if defined(PICORB_VM_MRUBYC)

#include "area512_hal.h"
#include "area512_repl.h"

void *
create_repl_row_sprite(void) {
  return area512_sprite_new_with_font_size(
    area512_gfx_width(),
    area512_console_row_height(),
    area512_console_font_size()
  );
}

void
draw_repl_input_line(
  void *row_sprite,
  const char *prompt,
  const ReplLine *repl_line
) {

  if (row_sprite == NULL)
    return;

  int prompt_pixel_width = area512_sprite_text_width(row_sprite, prompt);
  int cursor_pixel_width = area512_sprite_text_width(row_sprite, "_");

  int available_input_pixel_width =
    area512_gfx_width() - prompt_pixel_width - cursor_pixel_width;

  int visible_input_byte_offset = repl_line->input_cursor_byte_offset;
  char character_string[2] = { 0, 0 };
  int input_pixel_width = 0;

  while (visible_input_byte_offset > 0) {
    character_string[0] =
      repl_line->input_line[visible_input_byte_offset - 1];

    int character_pixel_width =
      area512_sprite_text_width(row_sprite, character_string);

    if (input_pixel_width + character_pixel_width > available_input_pixel_width)
      break;

    input_pixel_width += character_pixel_width;

    visible_input_byte_offset--;
  }

  int cursor_pixel_x = prompt_pixel_width + input_pixel_width;

  area512_sprite_fill(row_sprite, area512_theme_background_color());

  area512_sprite_text(
    row_sprite,
    0,
    0,
    prompt,
    area512_theme_text_color()
  );

  int visible_input_byte_count = 0;
  int character_pixel_x = prompt_pixel_width;

  while (
    (visible_input_byte_offset + visible_input_byte_count) <
    repl_line->input_byte_count
  ) {

    character_string[0] =
      repl_line->input_line[
        visible_input_byte_offset + visible_input_byte_count
      ];

    int character_pixel_width =
      area512_sprite_text_width(row_sprite, character_string);

    if (character_pixel_x + character_pixel_width > area512_gfx_width())
      break;

    area512_sprite_text(
      row_sprite,
      character_pixel_x,
      0,
      character_string,
      area512_theme_text_color()
    );

    character_pixel_x += character_pixel_width;
    visible_input_byte_count++;
  }

  area512_sprite_text(
    row_sprite,
    cursor_pixel_x,
    0,
    "_",
    area512_theme_selected_color()
  );

  area512_sprite_push(
    row_sprite,
    0,
    area512_console_cursor_row_index() * area512_console_row_height()
  );
}

#endif
