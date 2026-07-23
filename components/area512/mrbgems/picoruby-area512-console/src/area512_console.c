#include <stdbool.h>
#include <stdint.h>

#if defined(PICORB_VM_MRUBYC)

#include "area512_hal.h"
#include <mrubyc.h>

static void
c_console_reset(mrbc_vm *virtual_machine, mrbc_value *v, int argument_count) {

  (void)virtual_machine;
  (void)argument_count;

  area512_console_reset();

  SET_NIL_RETURN();
}

static void
c_console_wait_key_if_output(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int argument_count
) {

  (void)virtual_machine;
  (void)argument_count;

  if (area512_console_had_output())
    area512_console_getch_block();

  SET_NIL_RETURN();
}

void
mrbc_area512_console_init(mrbc_vm *virtual_machine) {
  mrbc_class *class_Console =
    mrbc_define_class(virtual_machine, "Console", mrbc_class_object);

  mrbc_define_method(virtual_machine, class_Console, "reset", c_console_reset);

  mrbc_define_method(
    virtual_machine,
    class_Console,
    "wait_key_if_output",
    c_console_wait_key_if_output
  );
}

#elif defined(PICORB_VM_MRUBY)

#include <mruby.h>

void
mrb_picoruby_area512_console_gem_init(mrb_state *state) {
  (void)state;
}

void
mrb_picoruby_area512_console_gem_final(mrb_state *state) {
  (void)state;
}

#endif
