// Extends picoruby's Sandbox class to run precompiled .mrb bytecode from
// the SD card. mruby/c executes bytecode in place instead of copying it, so
// the code (and any top irep it replaces) must outlive the task run; both are
// tracked on per-sandbox lists and freed only on an explicit release.
#if defined(PICORB_VM_MRUBYC)

#include "area512_hal.h"
#include "area512_sandbox.h"
#include "machine.h"

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

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

static uint8_t *s_suspend_vm_code = NULL;
static SandboxState *volatile s_running_sandbox_state = NULL;
static volatile bool s_is_signal_self_managed = false;
static volatile int s_terminating_signal_status = MACHINE_SIG_NONE;

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
read_whole_file(
  const char *path,
  size_t *size,
  const char **exception_message
) {

  char resolved_path[AREA512_PATH_MAX];

  if (
    area512_resolve_data_path(
      path,
      resolved_path,
      sizeof resolved_path
    ) != 0
  ) {

    *exception_message = "invalid path";
    return NULL;
  }

  FILE *file = fopen(resolved_path, "rb");

  if (!file) {
    *exception_message = strerror(errno);
    return NULL;
  }

  long file_size = -1;

  if (fseek(file, 0, SEEK_END) == 0) {
    file_size = ftell(file);
  }

  if (file_size < 0 || fseek(file, 0, SEEK_SET) != 0) {
    fclose(file);
    *exception_message = strerror(errno);

    return NULL;
  }

  uint8_t *buffer = (uint8_t *)mrbc_raw_alloc((unsigned int)file_size + 1);
  if (!buffer) {
    fclose(file);
    *exception_message = "out of memory";

    return NULL;
  }

  size_t read_size = fread(buffer, 1, (size_t)file_size, file);

  fclose(file);

  if (read_size != (size_t)file_size) {
    mrbc_raw_free(buffer);
    *exception_message = "read failed";

    return NULL;
  }

  buffer[read_size] = '\0';
  *size = read_size;

  return buffer;
}

static int
prepare_sandbox_task_for_run(
  SandboxState *sandbox_state,
  uint8_t *code,
  size_t size,
  const char **exception_message
) {

  // RITE0400: mruby bytecode container, format version 4.0.
  if (size < 12 || memcmp(code, "RITE0400", 8) != 0) {
    *exception_message = "not a valid mrb file";

    return 0;
  }

  Area512HeldCode *held_code =
    (Area512HeldCode *)mrbc_raw_alloc(sizeof(Area512HeldCode));

  if (!held_code) {
    *exception_message = "out of memory";

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
      *exception_message = "out of memory";

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
c_sandbox_area512_prepare_task_for_run(
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
  const char *exception_message = NULL;
  uint8_t *code = read_whole_file(path, &size, &exception_message);

  if (!code) {
    mrbc_raise(virtual_machine, MRBC_CLASS(RuntimeError), exception_message);
    return;
  }

  if (
    prepare_sandbox_task_for_run(
      sandbox_state,
      code,
      size,
      &exception_message
    )
  ) {
    SET_TRUE_RETURN();
    return;
  }

  mrbc_raw_free(code);

  if (exception_message) {
    mrbc_raise(virtual_machine, MRBC_CLASS(RuntimeError), exception_message);
    return;
  }

  SET_FALSE_RETURN();
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

static void
c_sandbox_area512_release_mrc_irep(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int argument_count
) {

  (void)virtual_machine;
  (void)argument_count;

  SANDBOX_STATE();

  if (sandbox_state->irep) {
    mrc_irep_free(sandbox_state->cc, sandbox_state->irep);
    sandbox_state->irep = NULL;
  }

  SET_NIL_RETURN();
}

static const char *
describe_sandbox_exception(const mrbc_exception *exception) {
  return exception->message
    ? (const char *)exception->message
    : mrbc_symid_to_str(exception->cls->sym_id);
}

static void
c_sandbox_area512_exception_message(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int argument_count
) {

  (void)argument_count;

  SANDBOX_STATE();

  mrbc_vm *sandbox_virtual_machine = (mrbc_vm *)&sandbox_state->tcb->vm;

  if (!mrbc_israised(sandbox_virtual_machine)) {
    SET_NIL_RETURN();

    return;
  }

  const char *message =
    describe_sandbox_exception(sandbox_virtual_machine->exception.exception);

  SET_RETURN(mrbc_string_new_cstr(virtual_machine, message));
}

static uint8_t *
compile_suspend_vm_code(void) {
  mrc_ccontext *context = mrc_ccontext_new(NULL);

  if (!context)
    return NULL;

  const uint8_t *script = (const uint8_t *)"Task.current.suspend";
  size_t script_byte_size = strlen((const char *)script);
  mrc_irep *irep = mrc_load_string_cxt(context, &script, script_byte_size);
  uint8_t *vm_code = NULL;
  size_t vm_code_byte_size = 0;
  int dump_result = MRC_DUMP_GENERAL_FAILURE;

  if (irep) {
    dump_result =
      mrc_dump_irep(context, irep, 0, &vm_code, &vm_code_byte_size);

    mrc_irep_free(context, irep);
  }

  if (context->options)
    pm_options_free(context->options);

  mrc_ccontext_free(context);

  return dump_result == MRC_DUMP_OK ? vm_code : NULL;
}

static bool
start_sandbox_task(SandboxState *sandbox_state, const char *task_name) {
  if (!s_suspend_vm_code)
    s_suspend_vm_code = compile_suspend_vm_code();

  if (!s_suspend_vm_code)
    return false;

  sandbox_state->tcb =
    mrbc_tcb_new(
      MAX_REGS_SIZE,
      MRBC_TASK_DEFAULT_STATE,
      MRBC_TASK_DEFAULT_PRIORITY
    );

  mrbc_create_task(s_suspend_vm_code, sandbox_state->tcb);
  sandbox_state->tcb->vm.flag_permanence = 1;
  mrbc_set_task_name(sandbox_state->tcb, task_name);

  return true;
}

static void
c_sandbox_new(mrbc_vm *virtual_machine, mrbc_value *v, int argument_count) {
  mrbc_value sandbox =
    mrbc_instance_new(virtual_machine, v->cls, sizeof(SandboxState));

  SandboxState *sandbox_state = (SandboxState *)sandbox.instance->data;
  memset(sandbox_state, 0, sizeof(SandboxState));

  const char *task_name =
    argument_count == 0 ? "sandbox" : (const char *)GET_STRING_ARG(1);

  if (!start_sandbox_task(sandbox_state, task_name)) {
    mrbc_raise(
      virtual_machine,
      MRBC_CLASS(RuntimeError),
      "failed to compile suspend_vm_code"
    );

    return;
  }

  SET_RETURN(sandbox);
}

static void
c_machine_signal_self_manage(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int argument_count
) {

  (void)virtual_machine;
  (void)argument_count;

  s_is_signal_self_managed = true;

  mrbc_value signal_self_manage_value = mrbc_true_value();

  mrbc_set_global(
    mrbc_str_to_symid("$_signal_self_manage"),
    &signal_self_manage_value
  );

  SET_TRUE_RETURN();
}

SandboxState *
area512_open_sandbox(const char *task_name) {
  s_is_signal_self_managed = false;
  s_terminating_signal_status = MACHINE_SIG_NONE;

  SandboxState *sandbox_state =
    (SandboxState *)mrbc_raw_alloc(sizeof(SandboxState));

  if (!sandbox_state)
    return NULL;

  memset(sandbox_state, 0, sizeof(SandboxState));

  if (!start_sandbox_task(sandbox_state, task_name)) {
    mrbc_raw_free(sandbox_state);

    return NULL;
  }

  return sandbox_state;
}

int
area512_run_sandbox_mrb_file(
  SandboxState *sandbox_state,
  const char *mrb_path,
  char *exception_message,
  int exception_message_size,
  const char **exception_class_name
) {

  mrbc_vm *sandbox_virtual_machine = (mrbc_vm *)&sandbox_state->tcb->vm;
  size_t size = 0;
  const char *load_exception_message = NULL;
  uint8_t *code = read_whole_file(mrb_path, &size, &load_exception_message);

  if (
    code &&
    !prepare_sandbox_task_for_run(
      sandbox_state,
      code,
      size,
      &load_exception_message
    )
  ) {

    mrbc_raw_free(code);
    code = NULL;
  }

  if (!code) {
    if (load_exception_message) {
      snprintf(
        exception_message,
        exception_message_size,
        "%s",
        load_exception_message
      );

      *exception_class_name = "RuntimeError";

    } else {
      const mrbc_exception *exception =
        sandbox_virtual_machine->exception.exception;

      snprintf(
        exception_message,
        exception_message_size,
        "%s",
        describe_sandbox_exception(exception)
      );

      *exception_class_name = mrbc_symid_to_str(exception->cls->sym_id);
    }

    return AREA512_SANDBOX_RUN_LOAD_FAILED;
  }

  s_running_sandbox_state = sandbox_state;

  mrbc_run();

  s_running_sandbox_state = NULL;

  if (s_terminating_signal_status != MACHINE_SIG_NONE) {
    if (s_terminating_signal_status == MACHINE_SIGINT_RECEIVED)
      mrbc_printf("^C\n");

    return AREA512_SANDBOX_RUN_INTERRUPTED;
  }

  if (mrbc_israised(sandbox_virtual_machine)) {
    const mrbc_exception *exception =
      sandbox_virtual_machine->exception.exception;

    snprintf(
      exception_message,
      exception_message_size,
      "%s",
      describe_sandbox_exception(exception)
    );

    *exception_class_name = mrbc_symid_to_str(exception->cls->sym_id);

    return AREA512_SANDBOX_RUN_RAISED;
  }

  return AREA512_SANDBOX_RUN_COMPLETED;
}

int
area512_run_sandbox_manifest(
  SandboxState *sandbox_state,
  const char *directory_path,
  const char *manifest_path,
  char *exception_message,
  int exception_message_size,
  const char **exception_class_name
) {

  size_t manifest_byte_size = 0;
  const char *load_exception_message = NULL;

  char *manifest_text =
    (char *)read_whole_file(
      manifest_path,
      &manifest_byte_size,
      &load_exception_message
    );

  if (!manifest_text) {
    snprintf(
      exception_message,
      exception_message_size,
      "%s",
      load_exception_message
    );

    *exception_class_name = "RuntimeError";

    return AREA512_SANDBOX_RUN_LOAD_FAILED;
  }

  char *manifest_cursor = manifest_text;
  char *entry_name;
  char entry_path[AREA512_PATH_MAX];
  int run_result = AREA512_SANDBOX_RUN_COMPLETED;

  while (
    run_result == AREA512_SANDBOX_RUN_COMPLETED &&
    (entry_name = area512_read_next_manifest_entry(&manifest_cursor)) != NULL
  ) {

    area512_join_manifest_entry_path(
      directory_path,
      entry_name,
      entry_path,
      sizeof entry_path
    );

    run_result =
      area512_run_sandbox_mrb_file(
        sandbox_state,
        entry_path,
        exception_message,
        exception_message_size,
        exception_class_name
      );
  }

  mrbc_raw_free(manifest_text);

  return run_result;
}

void
area512_clear_sandbox_pointers(void) {
  s_suspend_vm_code = NULL;
  s_held_codes = NULL;
  s_held_ireps = NULL;
  s_running_sandbox_state = NULL;
}

void
area512_terminate_sandbox_on_signal(void) {
  if (!s_running_sandbox_state || s_is_signal_self_managed)
    return;

  if (
    sigint_status != MACHINE_SIGINT_RECEIVED &&
    sigint_status != MACHINE_SIGTSTP_RECEIVED
  )
    return;

  s_terminating_signal_status = sigint_status;
  sigint_status = MACHINE_SIG_NONE;

  mrbc_terminate_task(s_running_sandbox_state->tcb);
}

// Area512.run_next(path): the filer runs this app as soon as the current one
// returns. An empty path cancels.
static void
c_area512_run_next(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int argument_count
) {

  if (argument_count != 1 || v[1].tt != MRBC_TT_STRING) {
    mrbc_raise(virtual_machine, MRBC_CLASS(TypeError), "expected String path");
    return;
  }

  if (area512_set_next_app((const char *)v[1].string->data) != 0) {
    mrbc_raise(virtual_machine, MRBC_CLASS(ArgumentError), "path too long");
    return;
  }

  SET_NIL_RETURN();
}

void
mrbc_area512_sandbox_init(mrbc_vm *virtual_machine) {
  // Defined here because every Ruby app run requires area512-sandbox, so apps
  // get Area512.run_next without a require of their own.
  mrbc_class *class_Area512 =
    mrbc_define_class(virtual_machine, "Area512", mrbc_class_object);

  mrbc_define_method(
    virtual_machine,
    class_Area512,
    "run_next",
    c_area512_run_next
  );

  mrbc_class *class_Sandbox = mrbc_get_class_by_name("Sandbox");

  if (!class_Sandbox)
    return;

  mrbc_define_method(virtual_machine, class_Sandbox, "new", c_sandbox_new);

  mrbc_class *module_Machine = mrbc_get_class_by_name("Machine");

  if (module_Machine) {
    mrbc_define_method(
      virtual_machine,
      module_Machine,
      "signal_self_manage",
      c_machine_signal_self_manage
    );
  }

  mrbc_define_method(
    virtual_machine,
    class_Sandbox,
    "area512_prepare_task_for_run",
    c_sandbox_area512_prepare_task_for_run
  );

  mrbc_define_method(
    virtual_machine,
    class_Sandbox,
    "area512_release_kept_code",
    c_sandbox_area512_release_kept_code
  );

  mrbc_define_method(
    virtual_machine,
    class_Sandbox,
    "area512_release_mrc_irep",
    c_sandbox_area512_release_mrc_irep
  );

  mrbc_define_method(
    virtual_machine,
    class_Sandbox,
    "area512_exception_message",
    c_sandbox_area512_exception_message
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
