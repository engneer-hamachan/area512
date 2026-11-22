#include <stdbool.h>
#include <stdint.h>

#if defined(PICORB_VM_MRUBYC)

#include "area512_hal.h"
#include <mrubyc.h>

#define SUPPORTED_FONT_SIZE_MESSAGE "font size must be 8, 10, 12, 14, 16, or 24"

static void *
fetch_sprite_handle(mrbc_value *v) {
  return *(void **)v->instance->data;
}

static void
delete_sprite_on_gc(mrbc_value *v) {
  void *sprite_handle = fetch_sprite_handle(v);

  if (sprite_handle != NULL)
    area512_sprite_delete(sprite_handle);
}

static bool
ensure_argument_count_or_raise(
  mrbc_vm *virtual_machine,
  int argument_count,
  int minimum
) {

  if (argument_count < minimum) {
    mrbc_raise(
      virtual_machine,
      MRBC_CLASS(ArgumentError),
      "wrong number of arguments"
    );

    return false;
  }

  return true;
}

static int
fetch_integer_or_raise(mrbc_vm *virtual_machine, mrbc_value *v, int index) {
  if (v[index].tt != MRBC_TT_INTEGER) {
    mrbc_raise(virtual_machine, MRBC_CLASS(TypeError), "expected Integer");
    return 0;
  }

  return (int)v[index].i;
}

static uint32_t
fetch_color_index_or_raise(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int index
) {

  int color_index = fetch_integer_or_raise(virtual_machine, v, index);

  if (color_index < 0 || color_index >= AREA512_QUATERNARY_SPRITE_COLOR_COUNT) {
    mrbc_raise(
      virtual_machine,
      MRBC_CLASS(ArgumentError),
      "expected a QuaternarySprite color index constant"
    );

    return AREA512_QUATERNARY_SPRITE_BACKGROUND_COLOR_INDEX;
  }

  return (uint32_t)color_index;
}

static bool
is_supported_font_size(int font_size) {
  return font_size == 8 || font_size == 10 || font_size == 12 ||
         font_size == 14 || font_size == 16 || font_size == 24;
}

static void
create_sprite_object(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int argument_count
) {

  if (argument_count < 2) {
    mrbc_raise(
      virtual_machine,
      MRBC_CLASS(ArgumentError),
      "wrong number of arguments (expected 2..3)"
    );

    return;
  }

  int width = fetch_integer_or_raise(virtual_machine, v, 1);
  int height = fetch_integer_or_raise(virtual_machine, v, 2);

  void *sprite_handle;

  if (argument_count >= 3) {
    int font_size = fetch_integer_or_raise(virtual_machine, v, 3);

    if (!is_supported_font_size(font_size)) {
      mrbc_raise(
        virtual_machine,
        MRBC_CLASS(ArgumentError),
        SUPPORTED_FONT_SIZE_MESSAGE
      );

      return;
    }

    sprite_handle =
      area512_quaternary_sprite_new_with_font_size(width, height, font_size);

  } else {
    sprite_handle = area512_quaternary_sprite_new(width, height);
  }

  if (sprite_handle == NULL) {
    mrbc_raise(
      virtual_machine,
      MRBC_CLASS(RuntimeError),
      "failed to allocate sprite"
    );

    return;
  }

  mrbc_value instance =
    mrbc_instance_new(virtual_machine, v->cls, sizeof(void *));

  *(void **)instance.instance->data = sprite_handle;

  SET_RETURN(instance);
}

static void
delete_sprite(mrbc_vm *virtual_machine, mrbc_value *v, int argument_count) {
  void *sprite_handle = fetch_sprite_handle(v);

  if (sprite_handle != NULL) {
    area512_sprite_delete(sprite_handle);

    *(void **)v->instance->data = NULL;
  }

  SET_NIL_RETURN();
}

static void
fetch_sprite_width(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int argument_count
) {

  SET_INT_RETURN(area512_sprite_width(fetch_sprite_handle(v)));
}

static void
fetch_sprite_height(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int argument_count
) {

  SET_INT_RETURN(area512_sprite_height(fetch_sprite_handle(v)));
}

static void
fill_sprite(mrbc_vm *virtual_machine, mrbc_value *v, int argument_count) {
  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 1))
    return;

  area512_sprite_fill(
    fetch_sprite_handle(v),
    fetch_color_index_or_raise(virtual_machine, v, 1)
  );
}

static void
draw_sprite_pixel(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int argument_count
) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 3))
    return;

  area512_sprite_pixel(
    fetch_sprite_handle(v),
    fetch_integer_or_raise(virtual_machine, v, 1),
    fetch_integer_or_raise(virtual_machine, v, 2),
    fetch_color_index_or_raise(virtual_machine, v, 3)
  );
}

static void
draw_sprite_line(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int argument_count
) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 5))
    return;

  area512_sprite_line(
    fetch_sprite_handle(v),
    fetch_integer_or_raise(virtual_machine, v, 1),
    fetch_integer_or_raise(virtual_machine, v, 2),
    fetch_integer_or_raise(virtual_machine, v, 3),
    fetch_integer_or_raise(virtual_machine, v, 4),
    fetch_color_index_or_raise(virtual_machine, v, 5)
  );
}

static void
draw_sprite_rectangle(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int argument_count
) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 5))
    return;

  area512_sprite_rect(
    fetch_sprite_handle(v),
    fetch_integer_or_raise(virtual_machine, v, 1),
    fetch_integer_or_raise(virtual_machine, v, 2),
    fetch_integer_or_raise(virtual_machine, v, 3),
    fetch_integer_or_raise(virtual_machine, v, 4),
    fetch_color_index_or_raise(virtual_machine, v, 5)
  );
}

static void
fill_sprite_rectangle(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int argument_count
) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 5))
    return;

  area512_sprite_fill_rect(
    fetch_sprite_handle(v),
    fetch_integer_or_raise(virtual_machine, v, 1),
    fetch_integer_or_raise(virtual_machine, v, 2),
    fetch_integer_or_raise(virtual_machine, v, 3),
    fetch_integer_or_raise(virtual_machine, v, 4),
    fetch_color_index_or_raise(virtual_machine, v, 5)
  );
}

static void
draw_sprite_circle(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int argument_count
) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 4))
    return;

  area512_sprite_circle(
    fetch_sprite_handle(v),
    fetch_integer_or_raise(virtual_machine, v, 1),
    fetch_integer_or_raise(virtual_machine, v, 2),
    fetch_integer_or_raise(virtual_machine, v, 3),
    fetch_color_index_or_raise(virtual_machine, v, 4)
  );
}

static void
fill_sprite_circle(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int argument_count
) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 4))
    return;

  area512_sprite_fill_circle(
    fetch_sprite_handle(v),
    fetch_integer_or_raise(virtual_machine, v, 1),
    fetch_integer_or_raise(virtual_machine, v, 2),
    fetch_integer_or_raise(virtual_machine, v, 3),
    fetch_color_index_or_raise(virtual_machine, v, 4)
  );
}

static void
draw_sprite_text(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int argument_count
) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 4))
    return;

  if (v[3].tt != MRBC_TT_STRING) {
    mrbc_raise(virtual_machine, MRBC_CLASS(TypeError), "expected String");
    return;
  }

  area512_sprite_text(
    fetch_sprite_handle(v),
    fetch_integer_or_raise(virtual_machine, v, 1),
    fetch_integer_or_raise(virtual_machine, v, 2),
    (const char *)v[3].string->data,
    fetch_color_index_or_raise(virtual_machine, v, 4)
  );
}

static void
push_sprite(mrbc_vm *virtual_machine, mrbc_value *v, int argument_count) {
  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 2))
    return;

  void *sprite_handle = fetch_sprite_handle(v);
  int x = fetch_integer_or_raise(virtual_machine, v, 1);
  int y = fetch_integer_or_raise(virtual_machine, v, 2);

  if (argument_count >= 3) {
    area512_sprite_push_transparent(
      sprite_handle,
      x,
      y,
      fetch_color_index_or_raise(virtual_machine, v, 3)
    );

  } else {
    area512_sprite_push(sprite_handle, x, y);
  }
}

static void
define_color_index_constant(
  mrbc_class *sprite_class,
  const char *name,
  int color_index
) {

  mrbc_value color_index_value = mrbc_integer_value(color_index);

  mrbc_set_class_const(
    sprite_class,
    mrbc_str_to_symid(name),
    &color_index_value
  );
}

void
mrbc_area512_quaternary_sprite_init(mrbc_vm *virtual_machine) {
  mrbc_class *sprite_class =
    mrbc_define_class(virtual_machine, "QuaternarySprite", mrbc_class_object);

  mrbc_define_destructor(sprite_class, delete_sprite_on_gc);

  mrbc_define_method(
    virtual_machine,
    sprite_class,
    "new",
    create_sprite_object
  );

  mrbc_define_method(virtual_machine, sprite_class, "delete", delete_sprite);

  mrbc_define_method(
    virtual_machine,
    sprite_class,
    "width",
    fetch_sprite_width
  );

  mrbc_define_method(
    virtual_machine,
    sprite_class,
    "height",
    fetch_sprite_height
  );

  mrbc_define_method(virtual_machine, sprite_class, "fill", fill_sprite);

  mrbc_define_method(
    virtual_machine,
    sprite_class,
    "pixel",
    draw_sprite_pixel
  );

  mrbc_define_method(virtual_machine, sprite_class, "line", draw_sprite_line);

  mrbc_define_method(
    virtual_machine,
    sprite_class,
    "rect",
    draw_sprite_rectangle
  );

  mrbc_define_method(
    virtual_machine,
    sprite_class,
    "fill_rect",
    fill_sprite_rectangle
  );

  mrbc_define_method(
    virtual_machine,
    sprite_class,
    "circle",
    draw_sprite_circle
  );

  mrbc_define_method(
    virtual_machine,
    sprite_class,
    "fill_circle",
    fill_sprite_circle
  );

  mrbc_define_method(virtual_machine, sprite_class, "text", draw_sprite_text);
  mrbc_define_method(virtual_machine, sprite_class, "push", push_sprite);

  define_color_index_constant(
    sprite_class,
    "BACKGROUND_COLOR_INDEX",
    AREA512_QUATERNARY_SPRITE_BACKGROUND_COLOR_INDEX
  );

  define_color_index_constant(
    sprite_class,
    "TEXT_COLOR_INDEX",
    AREA512_QUATERNARY_SPRITE_TEXT_COLOR_INDEX
  );

  define_color_index_constant(
    sprite_class,
    "EMPHASIS_COLOR_INDEX",
    AREA512_QUATERNARY_SPRITE_EMPHASIS_COLOR_INDEX
  );

  define_color_index_constant(
    sprite_class,
    "BORDER_COLOR_INDEX",
    AREA512_QUATERNARY_SPRITE_BORDER_COLOR_INDEX
  );

  define_color_index_constant(
    sprite_class,
    "SELECTED_COLOR_INDEX",
    AREA512_QUATERNARY_SPRITE_SELECTED_COLOR_INDEX
  );

  define_color_index_constant(
    sprite_class,
    "BOX_COLOR_INDEX",
    AREA512_QUATERNARY_SPRITE_BOX_COLOR_INDEX
  );
}

#elif defined(PICORB_VM_MRUBY)

// mruby path is unused (Area512 is mrubyc-only); empty stubs for the
// linker.
#include <mruby.h>

void
mrb_picoruby_area512_quaternary_sprite_gem_init(mrb_state *state) {
  (void)state;
}

void
mrb_picoruby_area512_quaternary_sprite_gem_final(mrb_state *state) {
  (void)state;
}

#endif
