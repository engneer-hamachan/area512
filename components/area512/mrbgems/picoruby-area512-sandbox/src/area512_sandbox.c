// Extends picoruby's Sandbox class to run precompiled .mrb bytecode from
// LittleFS. mruby/c executes bytecode in place instead of copying it, so the
// code (and any top irep it replaces) must outlive the task run; both are
// tracked on per-sandbox lists and freed only on an explicit release.
#if defined(PICORB_VM_MRUBYC)

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "littlefs.h"
#include <mruby_compiler.h>
#include <mrubyc.h>

// Mirrors the upstream Sandbox state struct (picoruby-sandbox/src/sandbox.c);
// the layout must match so v->instance->data reads the fields the upstream
// Sandbox allocated. Member names follow upstream and stay as-is.
typedef struct sandbox_state {
  mrc_ccontext *cc;
  mrc_irep *irep;
  mrbc_tcb *tcb;
  uint8_t *vm_code;
  pm_options_t *options;
} SandboxState;

typedef struct area512_held_code {
  SandboxState *sandbox_state;
  uint8_t *code;
  struct area512_held_code *next;
} Area512HeldCode;

typedef struct area512_held_irep {
  SandboxState *sandbox_state;
  mrbc_irep *irep;
  struct area512_held_irep *next;
} Area512HeldIrep;

// Bytecode and ireps still referenced by running sandboxes; freed by
// release_kept_code() once the task no longer needs them.
static Area512HeldCode *s_held_codes = NULL;
static Area512HeldIrep *s_held_ireps = NULL;

#define SANDBOX_STATE()                                                        \
  SandboxState *sandbox_state = (SandboxState *)v->instance->data

static void
reset_virtual_machine(mrbc_vm *virtual_machine) {
  virtual_machine->cur_irep = virtual_machine->top_irep;
  virtual_machine->inst = virtual_machine->cur_irep->inst;
  virtual_machine->cur_regs = virtual_machine->regs;
  virtual_machine->target_class = mrbc_class_object;
  virtual_machine->callinfo_tail = NULL;
  virtual_machine->ret_blk = NULL;
  virtual_machine->exception = mrbc_nil_value();
  virtual_machine->flag_preemption = 0;
  virtual_machine->flag_stop = 0;
}

static void
release_kept_code(SandboxState *sandbox_state) {
  Area512HeldCode **code_link = &s_held_codes;

  while (*code_link) {
    Area512HeldCode *held_code = *code_link;

    if (held_code->sandbox_state == sandbox_state) {
      *code_link = held_code->next;

      if (held_code->code)
        mrbc_raw_free(held_code->code);

      mrbc_raw_free(held_code);
    } else {
      code_link = &held_code->next;
    }
  }

  Area512HeldIrep **irep_link = &s_held_ireps;

  while (*irep_link) {
    Area512HeldIrep *held_irep = *irep_link;

    if (held_irep->sandbox_state == sandbox_state) {
      *irep_link = held_irep->next;

      if (held_irep->irep)
        mrbc_irep_free(held_irep->irep);

      mrbc_raw_free(held_irep);

    } else {
      irep_link = &held_irep->next;
    }
  }
}

static uint8_t *
read_lfs_file(mrbc_vm *virtual_machine, const char *path, size_t *size) {
  int error = littlefs_ensure_mounted();

  mrbc_raise_iff_lfs_error(virtual_machine, error, "lfs_mount");

  if (error < 0)
    return NULL;

  lfs_file_t file;
  error = lfs_file_open(littlefs_get_lfs(), &file, path, LFS_O_RDONLY);

  mrbc_raise_iff_lfs_error(virtual_machine, error, path);
  if (error < 0)
    return NULL;

  lfs_soff_t file_size = lfs_file_size(littlefs_get_lfs(), &file);
  if (file_size < 0) {
    lfs_file_close(littlefs_get_lfs(), &file);
    mrbc_raise_iff_lfs_error(virtual_machine, (int)file_size, "lfs_file_size");

    return NULL;
  }

  uint8_t *buffer = (uint8_t *)mrbc_raw_alloc((unsigned int)file_size + 1);
  if (!buffer) {
    lfs_file_close(littlefs_get_lfs(), &file);
    mrbc_raise(virtual_machine, MRBC_CLASS(RuntimeError), "out of memory");
    return NULL;
  }

  lfs_ssize_t read_size =
    lfs_file_read(littlefs_get_lfs(), &file, buffer, (lfs_size_t)file_size);

  error = lfs_file_close(littlefs_get_lfs(), &file);

  if (read_size < 0) {
    mrbc_raw_free(buffer);
    mrbc_raise_iff_lfs_error(virtual_machine, (int)read_size, "lfs_file_read");
    return NULL;
  }

  mrbc_raise_iff_lfs_error(virtual_machine, error, "lfs_file_close");
  if (error < 0) {
    mrbc_raw_free(buffer);

    return NULL;
  }

  buffer[read_size] = '\0';
  *size = (size_t)read_size;

  return buffer;
}

static int
execute_mrb_code_keep(
  mrbc_vm *virtual_machine,
  SandboxState *sandbox_state,
  uint8_t *code,
  size_t size
) {

  // RITE0400: mruby bytecode container, format version 4.0.
  if (size < 12 || memcmp(code, "RITE0400", 8) != 0) {
    mrbc_raise(
      virtual_machine,
      MRBC_CLASS(RuntimeError),
      "not a valid mrb file"
    );

    return 0;
  }

  Area512HeldCode *held_code =
    (Area512HeldCode *)mrbc_raw_alloc(sizeof(Area512HeldCode));

  if (!held_code) {
    mrbc_raise(virtual_machine, MRBC_CLASS(RuntimeError), "out of memory");

    return 0;
  }

  held_code->sandbox_state = sandbox_state;
  held_code->code = code;
  held_code->next = NULL;

  mrbc_vm *sandbox_virtual_machine = (mrbc_vm *)&sandbox_state->tcb->vm;
  mrbc_irep *prev_top_irep = sandbox_virtual_machine->top_irep;
  Area512HeldIrep *held_irep = NULL;

  if (prev_top_irep) {
    held_irep = (Area512HeldIrep *)mrbc_raw_alloc(sizeof(Area512HeldIrep));

    if (!held_irep) {
      mrbc_raw_free(held_code);
      mrbc_raise(virtual_machine, MRBC_CLASS(RuntimeError), "out of memory");

      return 0;
    }

    held_irep->sandbox_state = sandbox_state;
    held_irep->irep = prev_top_irep;
    held_irep->next = NULL;
  }

  sandbox_state->vm_code = code;
  if (mrbc_load_mrb(sandbox_virtual_machine, sandbox_state->vm_code) != 0) {
    if (held_irep)
      mrbc_raw_free(held_irep);

    mrbc_raw_free(held_code);

    return 0;
  }

  held_code->next = s_held_codes;
  s_held_codes = held_code;

  if (prev_top_irep && prev_top_irep != sandbox_virtual_machine->top_irep) {
    held_irep->next = s_held_ireps;
    s_held_ireps = held_irep;

  } else if (held_irep) {
    mrbc_raw_free(held_irep);
  }

  reset_virtual_machine(sandbox_virtual_machine);

  if (sandbox_state->tcb->state == TASKSTATE_DORMANT) {
    mrbc_start_task(sandbox_state->tcb);
  } else {
    mrbc_resume_task(sandbox_state->tcb);
  }

  return 1;
}

static void
c_sandbox_area512_exec_mrb_file_keep(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int argument_count
) {

  (void)argument_count;

  SANDBOX_STATE();

  if (v[1].tt != MRBC_TT_STRING) {
    mrbc_raise(virtual_machine, MRBC_CLASS(TypeError), "expected String path");

    return;
  }

  const char *path = (const char *)v[1].string->data;
  size_t size = 0;
  uint8_t *code = read_lfs_file(virtual_machine, path, &size);

  if (!code)
    return;

  if (execute_mrb_code_keep(virtual_machine, sandbox_state, code, size)) {
    SET_TRUE_RETURN();
  } else {
    mrbc_raw_free(code);
    SET_FALSE_RETURN();
  }
}

static void
c_sandbox_area512_release_kept_code(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int argument_count
) {

  (void)virtual_machine;
  (void)argument_count;

  SANDBOX_STATE();

  release_kept_code(sandbox_state);

  SET_NIL_RETURN();
}

void
mrbc_area512_sandbox_init(mrbc_vm *virtual_machine) {
  mrbc_class *class_Sandbox = mrbc_get_class_by_name("Sandbox");

  if (!class_Sandbox)
    return;

  mrbc_define_method(
    virtual_machine,
    class_Sandbox,
    "area512_exec_mrb_file_keep",
    c_sandbox_area512_exec_mrb_file_keep
  );

  mrbc_define_method(
    virtual_machine,
    class_Sandbox,
    "area512_release_kept_code",
    c_sandbox_area512_release_kept_code
  );
}

#elif defined(PICORB_VM_MRUBY)

#include <mruby.h>

void
mrb_picoruby_area512_sandbox_gem_init(mrb_state *state) {
  (void)state;
}

void
mrb_picoruby_area512_sandbox_gem_final(mrb_state *state) {
  (void)state;
}

#endif
