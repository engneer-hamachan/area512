// Sprite gem: bridges the area512_gfx.cpp extern "C" API to mrubyc classes
// Sprite (offscreen buffer) and Display (the shared screen). Colors are
// 0xRRGGBB.

#include <stdbool.h>
#include <stdint.h>

#if defined(PICORB_VM_MRUBYC)

#include "area512_hal.h"
#include <mrubyc.h>

static void *
fetch_sprite_handle(mrbc_value *v) {
  return *(void **)v->instance->data;
}

static void
mrbc_sprite_free(mrbc_value *v) {
  void *handle = *(void **)v->instance->data;
  if (handle) {
    area512_sprite_delete(handle);
  }
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
fetch_integer_or_raise(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int index
) {

  if (v[index].tt != MRBC_TT_INTEGER) {
    mrbc_raise(virtual_machine, MRBC_CLASS(TypeError), "expected Integer");
    return 0;
  }

  return (int)v[index].i;
}

static uint32_t
fetch_color_or_raise(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int index
) {

  return (uint32_t)fetch_integer_or_raise(virtual_machine, v, index);
}

static void
c_sprite_new(mrbc_vm *virtual_machine, mrbc_value *v, int argument_count) {
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

  void *handle;

  if (argument_count >= 3) {
    int font_size = fetch_integer_or_raise(virtual_machine, v, 3);

    if (font_size != 10 && font_size != 12) {
      mrbc_raise(
        virtual_machine,
        MRBC_CLASS(ArgumentError),
        "font size must be 10 or 12"
      );

      return;
    }

    handle = area512_sprite_new_with_font_size(width, height, font_size);

  } else {
    handle = area512_sprite_new(width, height);
  }

  if (handle == NULL) {
    mrbc_raise(
      virtual_machine,
      MRBC_CLASS(RuntimeError),
      "failed to allocate sprite"
    );

    return;
  }

  mrbc_value instance =
    mrbc_instance_new(virtual_machine, v->cls, sizeof(void *));

  *(void **)instance.instance->data = handle;

  SET_RETURN(instance);
}

static void
c_sprite_delete(mrbc_vm *virtual_machine, mrbc_value *v, int argument_count) {
  void *handle = fetch_sprite_handle(v);

  if (handle) {
    area512_sprite_delete(handle);
    *(void **)v->instance->data = NULL;
  }

  SET_NIL_RETURN();
}

static void
c_sprite_width(mrbc_vm *virtual_machine, mrbc_value *v, int argument_count) {
  SET_INT_RETURN(area512_sprite_width(fetch_sprite_handle(v)));
}

static void
c_sprite_height(mrbc_vm *virtual_machine, mrbc_value *v, int argument_count) {
  SET_INT_RETURN(area512_sprite_height(fetch_sprite_handle(v)));
}

static void
c_sprite_fill(
  mrbc_vm *virtual_machine,
  mrbc_value *values,
  int argument_count
) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 1)) {
    return;
  }

  area512_sprite_fill(
    fetch_sprite_handle(values),
    fetch_color_or_raise(virtual_machine, values, 1)
  );
}

static void
c_sprite_pixel(
  mrbc_vm *virtual_machine,
  mrbc_value *values,
  int argument_count
) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 3)) {
    return;
  }

  area512_sprite_pixel(
    fetch_sprite_handle(values),
    fetch_integer_or_raise(virtual_machine, values, 1),
    fetch_integer_or_raise(virtual_machine, values, 2),
    fetch_color_or_raise(virtual_machine, values, 3)
  );
}

static void
c_sprite_line(
  mrbc_vm *virtual_machine,
  mrbc_value *values,
  int argument_count
) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 5)) {
    return;
  }

  area512_sprite_line(
    fetch_sprite_handle(values),
    fetch_integer_or_raise(virtual_machine, values, 1),
    fetch_integer_or_raise(virtual_machine, values, 2),
    fetch_integer_or_raise(virtual_machine, values, 3),
    fetch_integer_or_raise(virtual_machine, values, 4),
    fetch_color_or_raise(virtual_machine, values, 5)
  );
}

static void
c_sprite_rect(
  mrbc_vm *virtual_machine,
  mrbc_value *values,
  int argument_count
) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 5)) {
    return;
  }

  area512_sprite_rect(
    fetch_sprite_handle(values),
    fetch_integer_or_raise(virtual_machine, values, 1),
    fetch_integer_or_raise(virtual_machine, values, 2),
    fetch_integer_or_raise(virtual_machine, values, 3),
    fetch_integer_or_raise(virtual_machine, values, 4),
    fetch_color_or_raise(virtual_machine, values, 5)
  );
}

static void
c_sprite_fill_rect(
  mrbc_vm *virtual_machine,
  mrbc_value *values,
  int argument_count
) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 5)) {
    return;
  }

  area512_sprite_fill_rect(
    fetch_sprite_handle(values),
    fetch_integer_or_raise(virtual_machine, values, 1),
    fetch_integer_or_raise(virtual_machine, values, 2),
    fetch_integer_or_raise(virtual_machine, values, 3),
    fetch_integer_or_raise(virtual_machine, values, 4),
    fetch_color_or_raise(virtual_machine, values, 5)
  );
}

static void
c_sprite_circle(
  mrbc_vm *virtual_machine,
  mrbc_value *values,
  int argument_count
) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 4)) {
    return;
  }

  area512_sprite_circle(
    fetch_sprite_handle(values),
    fetch_integer_or_raise(virtual_machine, values, 1),
    fetch_integer_or_raise(virtual_machine, values, 2),
    fetch_integer_or_raise(virtual_machine, values, 3),
    fetch_color_or_raise(virtual_machine, values, 4)
  );
}

static void
c_sprite_fill_circle(
  mrbc_vm *virtual_machine,
  mrbc_value *values,
  int argument_count
) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 4)) {
    return;
  }

  area512_sprite_fill_circle(
    fetch_sprite_handle(values),
    fetch_integer_or_raise(virtual_machine, values, 1),
    fetch_integer_or_raise(virtual_machine, values, 2),
    fetch_integer_or_raise(virtual_machine, values, 3),
    fetch_color_or_raise(virtual_machine, values, 4)
  );
}

static void
c_sprite_text(
  mrbc_vm *virtual_machine,
  mrbc_value *values,
  int argument_count
) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 4)) {
    return;
  }

  if (values[3].tt != MRBC_TT_STRING) {
    mrbc_raise(virtual_machine, MRBC_CLASS(TypeError), "expected String");
    return;
  }

  area512_sprite_text(
    fetch_sprite_handle(values),
    fetch_integer_or_raise(virtual_machine, values, 1),
    fetch_integer_or_raise(virtual_machine, values, 2),
    (const char *)values[3].string->data,
    fetch_color_or_raise(virtual_machine, values, 4)
  );
}

static void
c_sprite_push(
  mrbc_vm *virtual_machine,
  mrbc_value *values,
  int argument_count
) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 2)) {
    return;
  }

  void *handle = fetch_sprite_handle(values);
  int x = fetch_integer_or_raise(virtual_machine, values, 1);
  int y = fetch_integer_or_raise(virtual_machine, values, 2);

  if (argument_count >= 3) {
    area512_sprite_push_transparent(
      handle,
      x,
      y,
      fetch_color_or_raise(virtual_machine, values, 3)
    );

  } else {
    area512_sprite_push(handle, x, y);
  }
}

static void
c_display_width(mrbc_vm *virtual_machine, mrbc_value *v, int argument_count) {
  SET_INT_RETURN(area512_gfx_width());
}

static void
c_display_height(mrbc_vm *virtual_machine, mrbc_value *v, int argument_count) {
  SET_INT_RETURN(area512_gfx_height());
}

static void
c_display_fill_screen(
  mrbc_vm *virtual_machine,
  mrbc_value *values,
  int argument_count
) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 1)) {
    return;
  }

  area512_gfx_fill_screen(fetch_color_or_raise(virtual_machine, values, 1));
}

static void
c_display_brightness_eq(
  mrbc_vm *virtual_machine,
  mrbc_value *values,
  int argument_count
) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 1)) {
    return;
  }

  area512_gfx_set_brightness(
    fetch_integer_or_raise(virtual_machine, values, 1)
  );
}

static void
c_display_show_header_image(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int argument_count
) {

  if (!ensure_argument_count_or_raise(virtual_machine, argument_count, 1)) {
    return;
  }

  if (v[1].tt != MRBC_TT_STRING) {
    mrbc_raise(virtual_machine, MRBC_CLASS(TypeError), "expected String path");
    return;
  }

  int hold_milliseconds = 1000;
  if (argument_count >= 2) {
    hold_milliseconds = fetch_integer_or_raise(virtual_machine, v, 2);
  }

  int shown =
    area512_gfx_show_header_image(
      (const char *)v[1].string->data,
      hold_milliseconds
    );

  SET_BOOL_RETURN(shown);
}

void
mrbc_area512_sprite_init(mrbc_vm *virtual_machine) {
  mrbc_class *class_Sprite =
    mrbc_define_class(virtual_machine, "Sprite", mrbc_class_object);

  mrbc_define_destructor(class_Sprite, mrbc_sprite_free);
  mrbc_define_method(virtual_machine, class_Sprite, "new", c_sprite_new);
  mrbc_define_method(virtual_machine, class_Sprite, "delete", c_sprite_delete);
  mrbc_define_method(virtual_machine, class_Sprite, "width", c_sprite_width);
  mrbc_define_method(virtual_machine, class_Sprite, "height", c_sprite_height);
  mrbc_define_method(virtual_machine, class_Sprite, "fill", c_sprite_fill);
  mrbc_define_method(virtual_machine, class_Sprite, "pixel", c_sprite_pixel);
  mrbc_define_method(virtual_machine, class_Sprite, "line", c_sprite_line);
  mrbc_define_method(virtual_machine, class_Sprite, "rect", c_sprite_rect);

  mrbc_define_method(
    virtual_machine,
    class_Sprite,
    "fill_rect",
    c_sprite_fill_rect
  );

  mrbc_define_method(virtual_machine, class_Sprite, "circle", c_sprite_circle);

  mrbc_define_method(
    virtual_machine,
    class_Sprite,
    "fill_circle",
    c_sprite_fill_circle
  );

  mrbc_define_method(virtual_machine, class_Sprite, "text", c_sprite_text);
  mrbc_define_method(virtual_machine, class_Sprite, "push", c_sprite_push);

  mrbc_class *class_Display =
    mrbc_define_class(virtual_machine, "Display", mrbc_class_object);

  mrbc_define_method(virtual_machine, class_Display, "width", c_display_width);

  mrbc_define_method(
    virtual_machine,
    class_Display,
    "height",
    c_display_height
  );

  mrbc_define_method(
    virtual_machine,
    class_Display,
    "fill_screen",
    c_display_fill_screen
  );

  mrbc_define_method(
    virtual_machine,
    class_Display,
    "brightness=",
    c_display_brightness_eq
  );

  mrbc_define_method(
    virtual_machine,
    class_Display,
    "show_header_image",
    c_display_show_header_image
  );
}

#elif defined(PICORB_VM_MRUBY)

// mruby path is unused (Area512 is mrubyc-only); empty stubs for the
// linker.
#include <mruby.h>

void
mrb_picoruby_area512_sprite_gem_init(mrb_state *state) {
  (void)state;
}

void
mrb_picoruby_area512_sprite_gem_final(mrb_state *state) {
  (void)state;
}

#endif
