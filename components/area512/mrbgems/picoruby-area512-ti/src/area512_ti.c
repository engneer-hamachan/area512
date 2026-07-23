#if defined(PICORB_VM_MRUBYC)

#include <mrubyc.h>

void
mrbc_area512_ti_init(mrbc_vm *virtual_machine) {
  (void)virtual_machine;
}

#elif defined(PICORB_VM_MRUBY)

#include <mruby.h>

void
mrb_picoruby_area512_ti_gem_init(mrb_state *state) {
  (void)state;
}

void
mrb_picoruby_area512_ti_gem_final(mrb_state *state) {
  (void)state;
}

#endif
