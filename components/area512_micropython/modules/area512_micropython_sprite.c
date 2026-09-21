#include "area512_micropython_sprite.h"

#include "area512_hal.h"

#include "py/runtime.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  mp_obj_base_t base;
  void *sprite_handle;
} area512_micropython_sprite_instance_t;

void *
area512_micropython_fetch_sprite_handle_or_raise(mp_obj_t sprite_object) {
  if (!mp_obj_is_type(sprite_object, &area512_micropython_sprite_type) &&
      !mp_obj_is_type(sprite_object, &area512_micropython_banded_sprite_type))
    mp_raise_TypeError(MP_ERROR_TEXT("expected Sprite or BandedSprite"));

  area512_micropython_sprite_instance_t *sprite_instance =
    MP_OBJ_TO_PTR(sprite_object);

  if (sprite_instance->sprite_handle == NULL)
    mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("deleted sprite"));

  return sprite_instance->sprite_handle;
}

static uint32_t
fetch_color_or_raise(mp_obj_t color_object) {
  return (uint32_t)mp_obj_get_int(color_object);
}

#define SUPPORTED_FONT_SIZE_MESSAGE "font size must be 10, 12, 14, 16, or 24"

static bool
is_supported_font_size(int font_size) {
  return font_size == 10 || font_size == 12 || font_size == 14 ||
         font_size == 16 || font_size == 24;
}

static mp_obj_t
create_sprite_object(
  const mp_obj_type_t *sprite_type,
  size_t argument_count,
  size_t keyword_argument_count,
  const mp_obj_t *arguments
) {

  mp_arg_check_num(argument_count, keyword_argument_count, 2, 3, false);

  int width = mp_obj_get_int(arguments[0]);
  int height = mp_obj_get_int(arguments[1]);

  area512_micropython_sprite_instance_t *sprite_instance =
    mp_obj_malloc_with_finaliser(
      area512_micropython_sprite_instance_t,
      sprite_type
    );

  sprite_instance->sprite_handle = NULL;

  void *sprite_handle;

  if (argument_count >= 3 && arguments[2] != mp_const_none) {
    int font_size = mp_obj_get_int(arguments[2]);

    if (!is_supported_font_size(font_size))
      mp_raise_msg(
        &mp_type_ValueError,
        MP_ERROR_TEXT(SUPPORTED_FONT_SIZE_MESSAGE)
      );

    sprite_handle = area512_sprite_new_with_font_size(width, height, font_size);

  } else {
    sprite_handle = area512_sprite_new(width, height);
  }

  if (sprite_handle == NULL)
    mp_raise_type(&mp_type_MemoryError);

  sprite_instance->sprite_handle = sprite_handle;

  return MP_OBJ_FROM_PTR(sprite_instance);
}

static mp_obj_t
create_banded_sprite_object(
  const mp_obj_type_t *sprite_type,
  size_t argument_count,
  size_t keyword_argument_count,
  const mp_obj_t *arguments
) {

  mp_arg_check_num(argument_count, keyword_argument_count, 1, 1, false);
  int font_size = mp_obj_get_int(arguments[0]);

  if (!is_supported_font_size(font_size))
    mp_raise_msg(
      &mp_type_ValueError,
      MP_ERROR_TEXT(SUPPORTED_FONT_SIZE_MESSAGE)
    );

  area512_micropython_sprite_instance_t *sprite_instance =
    mp_obj_malloc_with_finaliser(
      area512_micropython_sprite_instance_t,
      sprite_type
    );

  sprite_instance->sprite_handle = NULL;

  void *sprite_handle = area512_screen_new(font_size);

  if (sprite_handle == NULL)
    mp_raise_msg(
      &mp_type_RuntimeError,
      MP_ERROR_TEXT("the screen buffer is already in use")
    );

  sprite_instance->sprite_handle = sprite_handle;

  return MP_OBJ_FROM_PTR(sprite_instance);
}

static mp_obj_t
draw_banded_sprite(mp_obj_t sprite_object) {
  return mp_obj_new_bool(area512_screen_draw(
    area512_micropython_fetch_sprite_handle_or_raise(sprite_object)
  ));
}
static MP_DEFINE_CONST_FUN_OBJ_1(
  draw_banded_sprite_callable,
  draw_banded_sprite
);

static mp_obj_t
fetch_sprite_region_top(mp_obj_t sprite_object) {
  return MP_OBJ_NEW_SMALL_INT(area512_screen_region_top(
    area512_micropython_fetch_sprite_handle_or_raise(sprite_object)
  ));
}
static MP_DEFINE_CONST_FUN_OBJ_1(
  fetch_sprite_region_top_callable,
  fetch_sprite_region_top
);

static mp_obj_t
fetch_sprite_region_bottom(mp_obj_t sprite_object) {
  return MP_OBJ_NEW_SMALL_INT(area512_screen_region_bottom(
    area512_micropython_fetch_sprite_handle_or_raise(sprite_object)
  ));
}
static MP_DEFINE_CONST_FUN_OBJ_1(
  fetch_sprite_region_bottom_callable,
  fetch_sprite_region_bottom
);

static mp_obj_t
delete_sprite(mp_obj_t sprite_object) {
  area512_micropython_sprite_instance_t *sprite_instance =
    MP_OBJ_TO_PTR(sprite_object);

  if (sprite_instance->sprite_handle != NULL) {
    area512_sprite_delete(sprite_instance->sprite_handle);

    sprite_instance->sprite_handle = NULL;
  }

  return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(delete_sprite_callable, delete_sprite);

static mp_obj_t
fetch_sprite_width(mp_obj_t sprite_object) {
  return MP_OBJ_NEW_SMALL_INT(area512_sprite_width(
    area512_micropython_fetch_sprite_handle_or_raise(sprite_object)
  ));
}
static MP_DEFINE_CONST_FUN_OBJ_1(
  fetch_sprite_width_callable,
  fetch_sprite_width
);

static mp_obj_t
fetch_sprite_height(mp_obj_t sprite_object) {
  return MP_OBJ_NEW_SMALL_INT(area512_sprite_height(
    area512_micropython_fetch_sprite_handle_or_raise(sprite_object)
  ));
}
static MP_DEFINE_CONST_FUN_OBJ_1(
  fetch_sprite_height_callable,
  fetch_sprite_height
);

static mp_obj_t
fill_sprite(mp_obj_t sprite_object, mp_obj_t color_object) {
  area512_sprite_fill(
    area512_micropython_fetch_sprite_handle_or_raise(sprite_object),
    fetch_color_or_raise(color_object)
  );

  return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(fill_sprite_callable, fill_sprite);

static mp_obj_t
draw_sprite_pixel(size_t argument_count, const mp_obj_t *arguments) {
  area512_sprite_pixel(
    area512_micropython_fetch_sprite_handle_or_raise(arguments[0]),
    mp_obj_get_int(arguments[1]),
    mp_obj_get_int(arguments[2]),
    fetch_color_or_raise(arguments[3])
  );

  return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(
  draw_sprite_pixel_callable,
  4,
  4,
  draw_sprite_pixel
);

static mp_obj_t
draw_sprite_line(size_t argument_count, const mp_obj_t *arguments) {
  area512_sprite_line(
    area512_micropython_fetch_sprite_handle_or_raise(arguments[0]),
    mp_obj_get_int(arguments[1]),
    mp_obj_get_int(arguments[2]),
    mp_obj_get_int(arguments[3]),
    mp_obj_get_int(arguments[4]),
    fetch_color_or_raise(arguments[5])
  );

  return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(
  draw_sprite_line_callable,
  6,
  6,
  draw_sprite_line
);

static mp_obj_t
draw_sprite_rectangle(size_t argument_count, const mp_obj_t *arguments) {
  area512_sprite_rect(
    area512_micropython_fetch_sprite_handle_or_raise(arguments[0]),
    mp_obj_get_int(arguments[1]),
    mp_obj_get_int(arguments[2]),
    mp_obj_get_int(arguments[3]),
    mp_obj_get_int(arguments[4]),
    fetch_color_or_raise(arguments[5])
  );

  return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(
  draw_sprite_rectangle_callable,
  6,
  6,
  draw_sprite_rectangle
);

static mp_obj_t
fill_sprite_rectangle(size_t argument_count, const mp_obj_t *arguments) {
  area512_sprite_fill_rect(
    area512_micropython_fetch_sprite_handle_or_raise(arguments[0]),
    mp_obj_get_int(arguments[1]),
    mp_obj_get_int(arguments[2]),
    mp_obj_get_int(arguments[3]),
    mp_obj_get_int(arguments[4]),
    fetch_color_or_raise(arguments[5])
  );

  return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(
  fill_sprite_rectangle_callable,
  6,
  6,
  fill_sprite_rectangle
);

static mp_obj_t
draw_sprite_circle(size_t argument_count, const mp_obj_t *arguments) {
  area512_sprite_circle(
    area512_micropython_fetch_sprite_handle_or_raise(arguments[0]),
    mp_obj_get_int(arguments[1]),
    mp_obj_get_int(arguments[2]),
    mp_obj_get_int(arguments[3]),
    fetch_color_or_raise(arguments[4])
  );

  return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(
  draw_sprite_circle_callable,
  5,
  5,
  draw_sprite_circle
);

static mp_obj_t
fill_sprite_circle(size_t argument_count, const mp_obj_t *arguments) {
  area512_sprite_fill_circle(
    area512_micropython_fetch_sprite_handle_or_raise(arguments[0]),
    mp_obj_get_int(arguments[1]),
    mp_obj_get_int(arguments[2]),
    mp_obj_get_int(arguments[3]),
    fetch_color_or_raise(arguments[4])
  );

  return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(
  fill_sprite_circle_callable,
  5,
  5,
  fill_sprite_circle
);

static mp_obj_t
draw_sprite_text(size_t argument_count, const mp_obj_t *arguments) {
  area512_sprite_text(
    area512_micropython_fetch_sprite_handle_or_raise(arguments[0]),
    mp_obj_get_int(arguments[1]),
    mp_obj_get_int(arguments[2]),
    mp_obj_str_get_str(arguments[3]),
    fetch_color_or_raise(arguments[4])
  );

  return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(
  draw_sprite_text_callable,
  5,
  5,
  draw_sprite_text
);

static mp_obj_t
push_sprite(size_t argument_count, const mp_obj_t *arguments) {
  void *sprite_handle =
    area512_micropython_fetch_sprite_handle_or_raise(arguments[0]);
  int x = mp_obj_get_int(arguments[1]);
  int y = mp_obj_get_int(arguments[2]);

  if (argument_count >= 4 && arguments[3] != mp_const_none)
    area512_sprite_push_transparent(
      sprite_handle,
      x,
      y,
      fetch_color_or_raise(arguments[3])
    );
  else
    area512_sprite_push(sprite_handle, x, y);

  return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(
  push_sprite_callable,
  3,
  4,
  push_sprite
);

static const mp_rom_map_elem_t sprite_locals_table[] = {
  {MP_ROM_QSTR(MP_QSTR_delete), MP_ROM_PTR(&delete_sprite_callable)},
  {MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&delete_sprite_callable)},
  {MP_ROM_QSTR(MP_QSTR_width), MP_ROM_PTR(&fetch_sprite_width_callable)},
  {MP_ROM_QSTR(MP_QSTR_height), MP_ROM_PTR(&fetch_sprite_height_callable)},
  {MP_ROM_QSTR(MP_QSTR_fill), MP_ROM_PTR(&fill_sprite_callable)},
  {MP_ROM_QSTR(MP_QSTR_pixel), MP_ROM_PTR(&draw_sprite_pixel_callable)},
  {MP_ROM_QSTR(MP_QSTR_line), MP_ROM_PTR(&draw_sprite_line_callable)},
  {MP_ROM_QSTR(MP_QSTR_rect), MP_ROM_PTR(&draw_sprite_rectangle_callable)},
  {MP_ROM_QSTR(MP_QSTR_fill_rect), MP_ROM_PTR(&fill_sprite_rectangle_callable)},
  {MP_ROM_QSTR(MP_QSTR_circle), MP_ROM_PTR(&draw_sprite_circle_callable)},
  {MP_ROM_QSTR(MP_QSTR_fill_circle), MP_ROM_PTR(&fill_sprite_circle_callable)},
  {MP_ROM_QSTR(MP_QSTR_text), MP_ROM_PTR(&draw_sprite_text_callable)},
  {MP_ROM_QSTR(MP_QSTR_push), MP_ROM_PTR(&push_sprite_callable)},
};
static MP_DEFINE_CONST_DICT(sprite_locals_dictionary, sprite_locals_table);

MP_DEFINE_CONST_OBJ_TYPE(
  area512_micropython_sprite_type,
  MP_QSTR_Sprite,
  MP_TYPE_FLAG_NONE,
  make_new,
  create_sprite_object,
  locals_dict,
  &sprite_locals_dictionary
);

static const mp_rom_map_elem_t banded_sprite_locals_table[] = {
  {MP_ROM_QSTR(MP_QSTR_delete), MP_ROM_PTR(&delete_sprite_callable)},
  {MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&delete_sprite_callable)},
  {MP_ROM_QSTR(MP_QSTR_width), MP_ROM_PTR(&fetch_sprite_width_callable)},
  {MP_ROM_QSTR(MP_QSTR_height), MP_ROM_PTR(&fetch_sprite_height_callable)},
  {MP_ROM_QSTR(MP_QSTR_fill), MP_ROM_PTR(&fill_sprite_callable)},
  {MP_ROM_QSTR(MP_QSTR_pixel), MP_ROM_PTR(&draw_sprite_pixel_callable)},
  {MP_ROM_QSTR(MP_QSTR_line), MP_ROM_PTR(&draw_sprite_line_callable)},
  {MP_ROM_QSTR(MP_QSTR_rect), MP_ROM_PTR(&draw_sprite_rectangle_callable)},
  {MP_ROM_QSTR(MP_QSTR_fill_rect), MP_ROM_PTR(&fill_sprite_rectangle_callable)},
  {MP_ROM_QSTR(MP_QSTR_circle), MP_ROM_PTR(&draw_sprite_circle_callable)},
  {MP_ROM_QSTR(MP_QSTR_fill_circle), MP_ROM_PTR(&fill_sprite_circle_callable)},
  {MP_ROM_QSTR(MP_QSTR_text), MP_ROM_PTR(&draw_sprite_text_callable)},
  {MP_ROM_QSTR(MP_QSTR_draw), MP_ROM_PTR(&draw_banded_sprite_callable)},
  {MP_ROM_QSTR(MP_QSTR_region_top),
   MP_ROM_PTR(&fetch_sprite_region_top_callable)},
  {MP_ROM_QSTR(MP_QSTR_region_bottom),
   MP_ROM_PTR(&fetch_sprite_region_bottom_callable)},
};
static MP_DEFINE_CONST_DICT(
  banded_sprite_locals_dictionary,
  banded_sprite_locals_table
);

MP_DEFINE_CONST_OBJ_TYPE(
  area512_micropython_banded_sprite_type,
  MP_QSTR_BandedSprite,
  MP_TYPE_FLAG_NONE,
  make_new,
  create_banded_sprite_object,
  locals_dict,
  &banded_sprite_locals_dictionary
);

static mp_obj_t
fetch_display_width(void) {
  return MP_OBJ_NEW_SMALL_INT(area512_gfx_width());
}
static MP_DEFINE_CONST_FUN_OBJ_0(
  fetch_display_width_callable,
  fetch_display_width
);

static mp_obj_t
fetch_display_height(void) {
  return MP_OBJ_NEW_SMALL_INT(area512_gfx_height());
}
static MP_DEFINE_CONST_FUN_OBJ_0(
  fetch_display_height_callable,
  fetch_display_height
);

static mp_obj_t
fill_display_screen(mp_obj_t color_object) {
  area512_gfx_fill_screen(fetch_color_or_raise(color_object));

  return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(
  fill_display_screen_callable,
  fill_display_screen
);

static mp_obj_t
set_display_brightness(mp_obj_t brightness_object) {
  area512_gfx_set_brightness(mp_obj_get_int(brightness_object));

  return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(
  set_display_brightness_callable,
  set_display_brightness
);

static mp_obj_t
show_display_header_image(size_t argument_count, const mp_obj_t *arguments) {
  int hold_milliseconds =
    argument_count >= 2 ? mp_obj_get_int(arguments[1]) : 1000;

  return mp_obj_new_bool(area512_gfx_show_header_image(
    mp_obj_str_get_str(arguments[0]),
    hold_milliseconds
  ));
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(
  show_display_header_image_callable,
  1,
  2,
  show_display_header_image
);

static const mp_rom_map_elem_t display_locals_table[] = {
  {MP_ROM_QSTR(MP_QSTR_width), MP_ROM_PTR(&fetch_display_width_callable)},
  {MP_ROM_QSTR(MP_QSTR_height), MP_ROM_PTR(&fetch_display_height_callable)},
  {MP_ROM_QSTR(MP_QSTR_fill_screen), MP_ROM_PTR(&fill_display_screen_callable)},
  {
    MP_ROM_QSTR(MP_QSTR_set_brightness),
    MP_ROM_PTR(&set_display_brightness_callable),
  },
  {
    MP_ROM_QSTR(MP_QSTR_show_header_image),
    MP_ROM_PTR(&show_display_header_image_callable),
  },
};
static MP_DEFINE_CONST_DICT(display_locals_dictionary, display_locals_table);

MP_DEFINE_CONST_OBJ_TYPE(
  area512_micropython_display_type,
  MP_QSTR_Display,
  MP_TYPE_FLAG_NONE,
  locals_dict,
  &display_locals_dictionary
);
