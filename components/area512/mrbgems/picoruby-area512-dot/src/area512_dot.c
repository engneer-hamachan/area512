#if defined(PICORB_VM_MRUBYC)

#include "core/dot_draw.h"
#include "core/dot_editor.h"
#include "core/dot_image.h"

#include "area512_hal.h"
#include <stdbool.h>
#include "io-console.h"
#include <mrubyc.h>

static DotImage *
fetch_receiver_dot_image(mrbc_value *values) {
  return *(DotImage **)values->instance->data;
}

static void *
fetch_sprite_handle_argument(
  mrbc_vm *virtual_machine,
  mrbc_value *values,
  int index
) {

  if (mrbc_type(values[index]) != MRBC_TT_OBJECT) {
    mrbc_raise(virtual_machine, MRBC_CLASS(TypeError), "expected Sprite");
    return NULL;
  }

  return *(void **)values[index].instance->data;
}

static DotImage *
fetch_dot_image_argument(
  mrbc_vm *virtual_machine,
  mrbc_value *values,
  int index
) {

  if (mrbc_type(values[index]) != MRBC_TT_OBJECT ||
      values[index].instance->cls != values->cls) {

    mrbc_raise(virtual_machine, MRBC_CLASS(TypeError), "expected Dot");
    return NULL;
  }

  return *(DotImage **)values[index].instance->data;
}

static void
raise_dot_image_status(mrbc_vm *virtual_machine, DotImageStatus status) {
  const char *message;

  switch (status) {
  case DOT_IMAGE_STATUS_OPEN_FAILED:
    message = "cannot open dot image";
    break;

  case DOT_IMAGE_STATUS_EMPTY_FILE:
    message = "empty dot image";
    break;

  case DOT_IMAGE_STATUS_MAGIC_MISMATCH:
    message = "not a dot image";
    break;

  case DOT_IMAGE_STATUS_VERSION_UNSUPPORTED:
    message = "unsupported dot image version";
    break;

  case DOT_IMAGE_STATUS_PALETTE_COLOR_COUNT_INVALID:
    message = "invalid palette color count";
    break;

  case DOT_IMAGE_STATUS_CONTENT_BYTE_LENGTH_SHORT:
    message = "dot image is truncated";
    break;

  case DOT_IMAGE_STATUS_OUT_OF_MEMORY:
    message = "failed to allocate dot image";
    break;

  case DOT_IMAGE_STATUS_WRITE_FAILED:
    message = "cannot write dot image";
    break;

  default:
    message = "dot image error";
    break;
  }

  mrbc_raise(virtual_machine, MRBC_CLASS(RuntimeError), message);
}

static void
c_dot_load(mrbc_vm *virtual_machine, mrbc_value *values, int argument_count) {
  if (argument_count < 1 || mrbc_type(values[1]) != MRBC_TT_STRING) {
    mrbc_raise(virtual_machine, MRBC_CLASS(ArgumentError), "expected a path");
    return;
  }

  const char *path = (const char *)values[1].string->data;
  int path_byte_length = values[1].string->size;
  DotImage *image = NULL;

  DotImageStatus status = load_dot_image_file(path, path_byte_length, &image);

  if (status != DOT_IMAGE_STATUS_OK) {
    raise_dot_image_status(virtual_machine, status);
    return;
  }

  mrbc_value instance =
    mrbc_instance_new(virtual_machine, values->cls, sizeof(DotImage *));

  *(DotImage **)instance.instance->data = image;

  mrbc_decref(values);

  values[0] = instance;
}

static void
c_dot_width(mrbc_vm *virtual_machine, mrbc_value *values, int argument_count) {
  DotImage *image = fetch_receiver_dot_image(values);

  mrbc_decref(values);
  mrbc_set_integer(values, image->width);
}

static void
c_dot_height(mrbc_vm *virtual_machine, mrbc_value *values, int argument_count) {
  DotImage *image = fetch_receiver_dot_image(values);

  mrbc_decref(values);
  mrbc_set_integer(values, image->height);
}

static void
c_dot_painted_left(
  mrbc_vm *virtual_machine,
  mrbc_value *values,
  int argument_count
) {

  DotImage *image = fetch_receiver_dot_image(values);

  mrbc_decref(values);
  mrbc_set_integer(values, image->painted_left);
}

static void
c_dot_painted_top(
  mrbc_vm *virtual_machine,
  mrbc_value *values,
  int argument_count
) {

  DotImage *image = fetch_receiver_dot_image(values);

  mrbc_decref(values);
  mrbc_set_integer(values, image->painted_top);
}

static void
c_dot_painted_right(
  mrbc_vm *virtual_machine,
  mrbc_value *values,
  int argument_count
) {

  DotImage *image = fetch_receiver_dot_image(values);

  mrbc_decref(values);
  mrbc_set_integer(values, image->painted_right);
}

static void
c_dot_painted_bottom(
  mrbc_vm *virtual_machine,
  mrbc_value *values,
  int argument_count
) {

  DotImage *image = fetch_receiver_dot_image(values);

  mrbc_decref(values);
  mrbc_set_integer(values, image->painted_bottom);
}

static void
c_dot_push(mrbc_vm *virtual_machine, mrbc_value *values, int argument_count) {
  if (argument_count < 3) {
    mrbc_raise(
      virtual_machine,
      MRBC_CLASS(ArgumentError),
      "wrong number of arguments (expected 3)"
    );

    return;
  }

  void *sprite = fetch_sprite_handle_argument(virtual_machine, values, 1);

  if (sprite == NULL)
    return;

  if (
    mrbc_type(values[2]) != MRBC_TT_INTEGER ||
    mrbc_type(values[3]) != MRBC_TT_INTEGER
  ) {

    mrbc_raise(virtual_machine, MRBC_CLASS(TypeError), "expected Integer");
    return;
  }

  DotImage *image = fetch_receiver_dot_image(values);
  int screen_x = (int)values[2].i;
  int screen_y = (int)values[3].i;

  push_dot_image_to_sprite(image, sprite, screen_x, screen_y);

  mrbc_decref(values);
  mrbc_set_nil(values);
}

static void
c_dot_is_overlap(
  mrbc_vm *virtual_machine,
  mrbc_value *values,
  int argument_count
) {

  if (argument_count < 6) {
    mrbc_raise(
      virtual_machine,
      MRBC_CLASS(ArgumentError),
      "wrong number of arguments (expected 6)"
    );

    return;
  }

  DotImage *first_image = fetch_dot_image_argument(virtual_machine, values, 1);

  if (first_image == NULL)
    return;

  DotImage *second_image = fetch_dot_image_argument(virtual_machine, values, 4);

  if (second_image == NULL)
    return;

  if (
    mrbc_type(values[2]) != MRBC_TT_INTEGER ||
    mrbc_type(values[3]) != MRBC_TT_INTEGER ||
    mrbc_type(values[5]) != MRBC_TT_INTEGER ||
    mrbc_type(values[6]) != MRBC_TT_INTEGER
  ) {

    mrbc_raise(virtual_machine, MRBC_CLASS(TypeError), "expected Integer");
    return;
  }

  int use_hitbox =
    argument_count >= 7 &&
    mrbc_type(values[7]) != MRBC_TT_FALSE &&
    mrbc_type(values[7]) != MRBC_TT_NIL;

  int has_overlap =
    has_dot_image_overlap(
      first_image,
      (int)values[2].i,
      (int)values[3].i,
      second_image,
      (int)values[5].i,
      (int)values[6].i,
      use_hitbox
    );

  mrbc_decref(values);
  mrbc_set_bool(values, has_overlap);
}

static void
c_dot_edit(mrbc_vm *virtual_machine, mrbc_value *values, int argument_count) {
  if (argument_count < 1 || mrbc_type(values[1]) != MRBC_TT_STRING) {
    mrbc_raise(virtual_machine, MRBC_CLASS(ArgumentError), "expected a path");
    return;
  }

  const char *path = (const char *)values[1].string->data;
  int path_byte_length = values[1].string->size;
  DotImage *image = NULL;

  DotImageStatus status = load_dot_image_file(path, path_byte_length, &image);

  if (status == DOT_IMAGE_STATUS_EMPTY_FILE) {
    status =
      create_blank_dot_image(DOT_EDITOR_WIDTH, DOT_EDITOR_HEIGHT, &image);
  }

  if (status != DOT_IMAGE_STATUS_OK) {
    raise_dot_image_status(virtual_machine, status);
    return;
  }

  if (
    image->width != DOT_EDITOR_WIDTH ||
    image->height != DOT_EDITOR_HEIGHT
  ) {

    free_dot_image(image);
    mrbc_raise(virtual_machine, MRBC_CLASS(RuntimeError), "Unsupported size");

    return;
  }

  void *screen_sprite =
    area512_sprite_new_with_font_size(
      area512_gfx_width(),
      area512_gfx_height(),
      DOT_EDITOR_FONT_SIZE
    );

  if (screen_sprite == NULL) {
    free_dot_image(image);

    mrbc_raise(
      virtual_machine,
      MRBC_CLASS(RuntimeError),
      "failed to allocate sprite"
    );

    return;
  }

  DotEditor editor;

  initialize_dot_editor(&editor, image, path, path_byte_length, screen_sprite);

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
  free_dot_image(image);

  if (status != DOT_IMAGE_STATUS_OK) {
    raise_dot_image_status(virtual_machine, status);
    return;
  }

  mrbc_decref(values);
  mrbc_set_nil(values);
}

static void
free_dot_image_on_gc(mrbc_value *self) {
  DotImage *image = *(DotImage **)self->instance->data;

  free_dot_image(image);
}

void
mrbc_area512_dot_init(mrbc_vm *virtual_machine) {
  mrbc_class *class_Dot =
    mrbc_define_class(virtual_machine, "Dot", mrbc_class_object);

  mrbc_define_destructor(class_Dot, free_dot_image_on_gc);
  mrbc_define_method(virtual_machine, class_Dot, "load", c_dot_load);
  mrbc_define_method(virtual_machine, class_Dot, "edit", c_dot_edit);
  mrbc_define_method(virtual_machine, class_Dot, "width", c_dot_width);
  mrbc_define_method(virtual_machine, class_Dot, "height", c_dot_height);
  mrbc_define_method(virtual_machine, class_Dot, "push", c_dot_push);
  mrbc_define_method(virtual_machine, class_Dot, "overlap?", c_dot_is_overlap);

  mrbc_define_method(
    virtual_machine,
    class_Dot,
    "painted_left",
    c_dot_painted_left
  );

  mrbc_define_method(
    virtual_machine,
    class_Dot,
    "painted_top",
    c_dot_painted_top
  );

  mrbc_define_method(
    virtual_machine,
    class_Dot,
    "painted_right",
    c_dot_painted_right
  );

  mrbc_define_method(
    virtual_machine,
    class_Dot,
    "painted_bottom",
    c_dot_painted_bottom
  );
}

#elif defined(PICORB_VM_MRUBY)

#include <mruby.h>

void
mrb_picoruby_area512_dot_gem_init(mrb_state *state) {
  (void)state;
}

void
mrb_picoruby_area512_dot_gem_final(mrb_state *state) {
  (void)state;
}

#endif
