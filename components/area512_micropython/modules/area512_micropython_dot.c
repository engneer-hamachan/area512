#include "area512_micropython_sprite.h"

#include "area512_hal.h"
#include "core/dot_draw.h"
#include "core/dot_editor.h"
#include "core/dot_image.h"

#include "py/runtime.h"

#include "io-console.h"

typedef struct {
  mp_obj_base_t base;
  DotImage *dot_image;
} area512_micropython_dot_instance_t;

static MP_NORETURN void
raise_dot_image_status(DotImageStatus status) {
  switch (status) {
  case DOT_IMAGE_STATUS_OPEN_FAILED:
    mp_raise_msg(
      &mp_type_OSError,
      MP_ERROR_TEXT("cannot open dot image")
    );

  case DOT_IMAGE_STATUS_EMPTY_FILE:
    mp_raise_msg(
      &mp_type_ValueError,
      MP_ERROR_TEXT("empty dot image")
    );

  case DOT_IMAGE_STATUS_MAGIC_MISMATCH:
    mp_raise_msg(
      &mp_type_ValueError,
      MP_ERROR_TEXT("not a dot image")
    );

  case DOT_IMAGE_STATUS_VERSION_UNSUPPORTED:
    mp_raise_msg(
      &mp_type_ValueError,
      MP_ERROR_TEXT("unsupported dot image version")
    );

  case DOT_IMAGE_STATUS_PALETTE_COLOR_COUNT_INVALID:
    mp_raise_msg(
      &mp_type_ValueError,
      MP_ERROR_TEXT("invalid palette color count")
    );

  case DOT_IMAGE_STATUS_CONTENT_BYTE_LENGTH_SHORT:
    mp_raise_msg(
      &mp_type_ValueError,
      MP_ERROR_TEXT("dot image is truncated")
    );

  case DOT_IMAGE_STATUS_OUT_OF_MEMORY:
    mp_raise_type(&mp_type_MemoryError);

  case DOT_IMAGE_STATUS_WRITE_FAILED:
    mp_raise_msg(
      &mp_type_OSError,
      MP_ERROR_TEXT("cannot write dot image")
    );

  default:
    mp_raise_msg(
      &mp_type_RuntimeError,
      MP_ERROR_TEXT("dot image error")
    );
  }
}

static DotImage *
fetch_dot_image_or_raise(mp_obj_t dot_object) {
  if (!mp_obj_is_type(dot_object, &area512_micropython_dot_type))
    mp_raise_TypeError(MP_ERROR_TEXT("expected Dot"));

  area512_micropython_dot_instance_t *dot_instance = MP_OBJ_TO_PTR(dot_object);

  if (dot_instance->dot_image == NULL)
    mp_raise_msg(
      &mp_type_ValueError,
      MP_ERROR_TEXT("deleted dot image")
    );

  return dot_instance->dot_image;
}

static mp_obj_t
load_dot_object(mp_obj_t path_object) {
  size_t path_byte_length;

  const char *path =
    mp_obj_str_get_data(path_object, &path_byte_length);

  area512_micropython_dot_instance_t *dot_instance =
    mp_obj_malloc_with_finaliser(
      area512_micropython_dot_instance_t,
      &area512_micropython_dot_type
    );

  dot_instance->dot_image = NULL;

  DotImage *dot_image = NULL;

  DotImageStatus status =
    load_dot_image_file(path, (int)path_byte_length, &dot_image);

  if (status != DOT_IMAGE_STATUS_OK)
    raise_dot_image_status(status);

  dot_instance->dot_image = dot_image;

  return MP_OBJ_FROM_PTR(dot_instance);
}
static MP_DEFINE_CONST_FUN_OBJ_1(load_dot_object_callable, load_dot_object);

static mp_obj_t
fetch_dot_width(mp_obj_t dot_object) {
  return MP_OBJ_NEW_SMALL_INT(fetch_dot_image_or_raise(dot_object)->width);
}
static MP_DEFINE_CONST_FUN_OBJ_1(fetch_dot_width_callable, fetch_dot_width);

static mp_obj_t
fetch_dot_height(mp_obj_t dot_object) {
  return MP_OBJ_NEW_SMALL_INT(fetch_dot_image_or_raise(dot_object)->height);
}
static MP_DEFINE_CONST_FUN_OBJ_1(fetch_dot_height_callable, fetch_dot_height);

static mp_obj_t
fetch_dot_painted_left(mp_obj_t dot_object) {
  return MP_OBJ_NEW_SMALL_INT(
    fetch_dot_image_or_raise(dot_object)->painted_left
  );
}
static MP_DEFINE_CONST_FUN_OBJ_1(
  fetch_dot_painted_left_callable,
  fetch_dot_painted_left
);

static mp_obj_t
fetch_dot_painted_top(mp_obj_t dot_object) {
  return MP_OBJ_NEW_SMALL_INT(
    fetch_dot_image_or_raise(dot_object)->painted_top
  );
}
static MP_DEFINE_CONST_FUN_OBJ_1(
  fetch_dot_painted_top_callable,
  fetch_dot_painted_top
);

static mp_obj_t
fetch_dot_painted_right(mp_obj_t dot_object) {
  return MP_OBJ_NEW_SMALL_INT(
    fetch_dot_image_or_raise(dot_object)->painted_right
  );
}
static MP_DEFINE_CONST_FUN_OBJ_1(
  fetch_dot_painted_right_callable,
  fetch_dot_painted_right
);

static mp_obj_t
fetch_dot_painted_bottom(mp_obj_t dot_object) {
  return MP_OBJ_NEW_SMALL_INT(
    fetch_dot_image_or_raise(dot_object)->painted_bottom
  );
}
static MP_DEFINE_CONST_FUN_OBJ_1(
  fetch_dot_painted_bottom_callable,
  fetch_dot_painted_bottom
);

static mp_obj_t
push_dot_to_sprite(size_t argument_count, const mp_obj_t *arguments) {
  push_dot_image_to_sprite(
    fetch_dot_image_or_raise(arguments[0]),
    area512_micropython_fetch_sprite_handle_or_raise(
      arguments[1]
    ),
    mp_obj_get_int(arguments[2]),
    mp_obj_get_int(arguments[3])
  );

  return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(
  push_dot_to_sprite_callable,
  4,
  4,
  push_dot_to_sprite
);

static mp_obj_t
has_dot_overlap(size_t argument_count, const mp_obj_t *arguments) {
  const DotImage *first_dot_image = fetch_dot_image_or_raise(arguments[0]);
  const DotImage *second_dot_image = fetch_dot_image_or_raise(arguments[3]);
  int use_hitbox = argument_count >= 7 && mp_obj_is_true(arguments[6]);

  int has_overlap = has_dot_image_overlap(
    first_dot_image,
    mp_obj_get_int(arguments[1]),
    mp_obj_get_int(arguments[2]),
    second_dot_image,
    mp_obj_get_int(arguments[4]),
    mp_obj_get_int(arguments[5]),
    use_hitbox
  );

  return mp_obj_new_bool(has_overlap);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(
  has_dot_overlap_callable,
  6,
  7,
  has_dot_overlap
);

static mp_obj_t
edit_dot_image_file(mp_obj_t path_object) {
  size_t path_byte_length;

  const char *path =
    mp_obj_str_get_data(path_object, &path_byte_length);

  DotImage *dot_image = NULL;

  DotImageStatus status =
    load_dot_image_file(path, (int)path_byte_length, &dot_image);

  if (status == DOT_IMAGE_STATUS_EMPTY_FILE)
    status =
      create_blank_dot_image(DOT_EDITOR_WIDTH, DOT_EDITOR_HEIGHT, &dot_image);

  if (status != DOT_IMAGE_STATUS_OK)
    raise_dot_image_status(status);

  if (
    dot_image->width != DOT_EDITOR_WIDTH ||
    dot_image->height != DOT_EDITOR_HEIGHT
  ) {

    free_dot_image(dot_image);

    mp_raise_msg(
      &mp_type_ValueError,
      MP_ERROR_TEXT("unsupported size")
    );
  }

  void *screen_sprite =
    area512_sprite_new_with_font_size(
      area512_gfx_width(),
      area512_gfx_height(),
      DOT_EDITOR_FONT_SIZE
    );

  if (screen_sprite == NULL) {
    free_dot_image(dot_image);

    mp_raise_type(&mp_type_MemoryError);
  }

  DotEditor editor;

  initialize_dot_editor(
    &editor,
    dot_image,
    path,
    (int)path_byte_length,
    screen_sprite
  );

  io_raw_bang(false);

  for (;;) {
    draw_dot_editor_screen(&editor);

    int key_byte = area512_console_getch_block();

    if (key_byte < 0)
      continue;

    DotEditorStatus editor_status = handle_dot_editor_key(&editor, key_byte);

    if (editor_status == DOT_EDITOR_STATUS_QUIT)
      break;

    if (editor_status == DOT_EDITOR_STATUS_SAVE) {
      status =
        save_dot_image_file(editor.image, editor.path, editor.path_byte_length);

      if (status != DOT_IMAGE_STATUS_OK)
        break;
    }
  }

  io_cooked_bang();

  area512_sprite_delete(screen_sprite);
  free_dot_image(dot_image);

  if (status != DOT_IMAGE_STATUS_OK)
    raise_dot_image_status(status);

  return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(
  edit_dot_image_file_callable,
  edit_dot_image_file
);

static mp_obj_t
free_dot_image_on_gc(mp_obj_t dot_object) {
  area512_micropython_dot_instance_t *dot_instance = MP_OBJ_TO_PTR(dot_object);

  if (dot_instance->dot_image != NULL) {
    free_dot_image(dot_instance->dot_image);

    dot_instance->dot_image = NULL;
  }

  return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(
  free_dot_image_on_gc_callable,
  free_dot_image_on_gc
);

static const mp_rom_map_elem_t dot_locals_table[] = {
  {MP_ROM_QSTR(MP_QSTR_load), MP_ROM_PTR(&load_dot_object_callable)},
  {MP_ROM_QSTR(MP_QSTR_edit), MP_ROM_PTR(&edit_dot_image_file_callable)},
  {MP_ROM_QSTR(MP_QSTR_overlap), MP_ROM_PTR(&has_dot_overlap_callable)},
  {MP_ROM_QSTR(MP_QSTR_width), MP_ROM_PTR(&fetch_dot_width_callable)},
  {MP_ROM_QSTR(MP_QSTR_height), MP_ROM_PTR(&fetch_dot_height_callable)},
  {
    MP_ROM_QSTR(MP_QSTR_painted_left),
    MP_ROM_PTR(&fetch_dot_painted_left_callable),
  },
  {
    MP_ROM_QSTR(MP_QSTR_painted_top),
    MP_ROM_PTR(&fetch_dot_painted_top_callable),
  },
  {
    MP_ROM_QSTR(MP_QSTR_painted_right),
    MP_ROM_PTR(&fetch_dot_painted_right_callable),
  },
  {
    MP_ROM_QSTR(MP_QSTR_painted_bottom),
    MP_ROM_PTR(&fetch_dot_painted_bottom_callable),
  },
  {MP_ROM_QSTR(MP_QSTR_push), MP_ROM_PTR(&push_dot_to_sprite_callable)},
  {MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&free_dot_image_on_gc_callable)},
};
static MP_DEFINE_CONST_DICT(dot_locals_dictionary, dot_locals_table);

MP_DEFINE_CONST_OBJ_TYPE(
  area512_micropython_dot_type,
  MP_QSTR_Dot,
  MP_TYPE_FLAG_NONE,
  locals_dict,
  &dot_locals_dictionary
);
