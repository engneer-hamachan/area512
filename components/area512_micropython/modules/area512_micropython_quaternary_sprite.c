#include "area512_hal.h"
#include "area512_micropython_sprite.h"

#include "py/runtime.h"

#include <stdbool.h>
#include <stdint.h>

#define SUPPORTED_FONT_SIZE_MESSAGE "font size must be 8, 10, 12, 14, 16, or 24"

typedef struct {
  mp_obj_base_t base;
  void *sprite_handle;
} quaternary_sprite_instance_t;

static void *
fetch_sprite_handle_or_raise(mp_obj_t sprite_object) {
  if (
    !mp_obj_is_type(
      sprite_object,
      &area512_micropython_quaternary_sprite_type
    )
  )
    mp_raise_TypeError(MP_ERROR_TEXT("expected QuaternarySprite"));

  quaternary_sprite_instance_t *sprite_instance = MP_OBJ_TO_PTR(sprite_object);

  if (sprite_instance->sprite_handle == NULL)
    mp_raise_msg(
      &mp_type_ValueError,
      MP_ERROR_TEXT("deleted sprite")
    );

  return sprite_instance->sprite_handle;
}

void *
area512_micropython_fetch_quaternary_sprite_handle_or_raise(
  mp_obj_t sprite_object
) {

  return fetch_sprite_handle_or_raise(sprite_object);
}

static uint32_t
fetch_color_index_or_raise(mp_obj_t color_index_object) {
  mp_int_t color_index = mp_obj_get_int(color_index_object);

  if (color_index < 0 || color_index >= AREA512_QUATERNARY_SPRITE_COLOR_COUNT)
    mp_raise_msg(
      &mp_type_ValueError,
      MP_ERROR_TEXT("expected a QuaternarySprite color index constant")
    );

  return (uint32_t)color_index;
}

static bool
is_supported_font_size(int font_size) {
  return font_size == 8 || font_size == 10 || font_size == 12 ||
         font_size == 14 || font_size == 16 || font_size == 24;
}

static mp_obj_t
create_sprite_object(
  const mp_obj_type_t *sprite_type,
  size_t argument_count,
  size_t keyword_argument_count,
  const mp_obj_t *arguments
) {

  mp_arg_check_num(
    argument_count,
    keyword_argument_count,
    2,
    3,
    false
  );

  int width = mp_obj_get_int(arguments[0]);
  int height = mp_obj_get_int(arguments[1]);

  quaternary_sprite_instance_t *sprite_instance =
    mp_obj_malloc_with_finaliser(
      quaternary_sprite_instance_t,
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

    sprite_handle =
      area512_quaternary_sprite_new_with_font_size(width, height, font_size);

  } else {
    sprite_handle = area512_quaternary_sprite_new(width, height);
  }

  if (sprite_handle == NULL)
    mp_raise_type(&mp_type_MemoryError);

  sprite_instance->sprite_handle = sprite_handle;

  return MP_OBJ_FROM_PTR(sprite_instance);
}

static mp_obj_t
delete_sprite(mp_obj_t sprite_object) {
  quaternary_sprite_instance_t *sprite_instance = MP_OBJ_TO_PTR(sprite_object);

  if (sprite_instance->sprite_handle != NULL) {
    area512_sprite_delete(sprite_instance->sprite_handle);

    sprite_instance->sprite_handle = NULL;
  }

  return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(delete_sprite_callable, delete_sprite);

static mp_obj_t
fetch_sprite_width(mp_obj_t sprite_object) {
  return MP_OBJ_NEW_SMALL_INT(
    area512_sprite_width(fetch_sprite_handle_or_raise(sprite_object))
  );
}
static MP_DEFINE_CONST_FUN_OBJ_1(
  fetch_sprite_width_callable,
  fetch_sprite_width
);

static mp_obj_t
fetch_sprite_height(mp_obj_t sprite_object) {
  return MP_OBJ_NEW_SMALL_INT(
    area512_sprite_height(fetch_sprite_handle_or_raise(sprite_object))
  );
}
static MP_DEFINE_CONST_FUN_OBJ_1(
  fetch_sprite_height_callable,
  fetch_sprite_height
);

static mp_obj_t
fill_sprite(mp_obj_t sprite_object, mp_obj_t color_index_object) {
  area512_sprite_fill(
    fetch_sprite_handle_or_raise(sprite_object),
    fetch_color_index_or_raise(color_index_object)
  );

  return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(fill_sprite_callable, fill_sprite);

static mp_obj_t
draw_sprite_pixel(size_t argument_count, const mp_obj_t *arguments) {
  area512_sprite_pixel(
    fetch_sprite_handle_or_raise(arguments[0]),
    mp_obj_get_int(arguments[1]),
    mp_obj_get_int(arguments[2]),
    fetch_color_index_or_raise(arguments[3])
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
    fetch_sprite_handle_or_raise(arguments[0]),
    mp_obj_get_int(arguments[1]),
    mp_obj_get_int(arguments[2]),
    mp_obj_get_int(arguments[3]),
    mp_obj_get_int(arguments[4]),
    fetch_color_index_or_raise(arguments[5])
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
    fetch_sprite_handle_or_raise(arguments[0]),
    mp_obj_get_int(arguments[1]),
    mp_obj_get_int(arguments[2]),
    mp_obj_get_int(arguments[3]),
    mp_obj_get_int(arguments[4]),
    fetch_color_index_or_raise(arguments[5])
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
    fetch_sprite_handle_or_raise(arguments[0]),
    mp_obj_get_int(arguments[1]),
    mp_obj_get_int(arguments[2]),
    mp_obj_get_int(arguments[3]),
    mp_obj_get_int(arguments[4]),
    fetch_color_index_or_raise(arguments[5])
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
    fetch_sprite_handle_or_raise(arguments[0]),
    mp_obj_get_int(arguments[1]),
    mp_obj_get_int(arguments[2]),
    mp_obj_get_int(arguments[3]),
    fetch_color_index_or_raise(arguments[4])
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
    fetch_sprite_handle_or_raise(arguments[0]),
    mp_obj_get_int(arguments[1]),
    mp_obj_get_int(arguments[2]),
    mp_obj_get_int(arguments[3]),
    fetch_color_index_or_raise(arguments[4])
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
    fetch_sprite_handle_or_raise(arguments[0]),
    mp_obj_get_int(arguments[1]),
    mp_obj_get_int(arguments[2]),
    mp_obj_str_get_str(arguments[3]),
    fetch_color_index_or_raise(arguments[4])
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
    fetch_sprite_handle_or_raise(arguments[0]);

  int x = mp_obj_get_int(arguments[1]);
  int y = mp_obj_get_int(arguments[2]);

  if (argument_count >= 4 && arguments[3] != mp_const_none)
    area512_sprite_push_transparent(
      sprite_handle,
      x,
      y,
      fetch_color_index_or_raise(arguments[3])
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
  {
    MP_ROM_QSTR(MP_QSTR_BACKGROUND_COLOR_INDEX),
    MP_ROM_INT(AREA512_QUATERNARY_SPRITE_BACKGROUND_COLOR_INDEX),
  },
  {
    MP_ROM_QSTR(MP_QSTR_TEXT_COLOR_INDEX),
    MP_ROM_INT(AREA512_QUATERNARY_SPRITE_TEXT_COLOR_INDEX),
  },
  {
    MP_ROM_QSTR(MP_QSTR_EMPHASIS_COLOR_INDEX),
    MP_ROM_INT(AREA512_QUATERNARY_SPRITE_EMPHASIS_COLOR_INDEX),
  },
  {
    MP_ROM_QSTR(MP_QSTR_BORDER_COLOR_INDEX),
    MP_ROM_INT(AREA512_QUATERNARY_SPRITE_BORDER_COLOR_INDEX),
  },
  {
    MP_ROM_QSTR(MP_QSTR_SELECTED_COLOR_INDEX),
    MP_ROM_INT(AREA512_QUATERNARY_SPRITE_SELECTED_COLOR_INDEX),
  },
  {
    MP_ROM_QSTR(MP_QSTR_BOX_COLOR_INDEX),
    MP_ROM_INT(AREA512_QUATERNARY_SPRITE_BOX_COLOR_INDEX),
  },
};
static MP_DEFINE_CONST_DICT(sprite_locals_dictionary, sprite_locals_table);

MP_DEFINE_CONST_OBJ_TYPE(
  area512_micropython_quaternary_sprite_type,
  MP_QSTR_QuaternarySprite,
  MP_TYPE_FLAG_NONE,
  make_new,
  create_sprite_object,
  locals_dict,
  &sprite_locals_dictionary
);
