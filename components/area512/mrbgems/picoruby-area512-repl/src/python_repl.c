#if defined(PICORB_VM_MRUBYC)

#include "area512_hal.h"
#include "area512_micropython.h"
#include "area512_repl.h"
#include "python_repl.h"

#include <string.h>

#include <mrubyc.h>

#define PYTHON_REPL_PRIMARY_PROMPT ">>> "
#define PYTHON_REPL_CONTINUATION_PROMPT "... "
#define PYTHON_REPL_SOURCE_INITIAL_BYTE_CAPACITY 64
#define OUT_OF_MEMORY_MESSAGE "Out of memory"

typedef struct {
  char *source_text;
  int source_byte_count;
  int source_byte_capacity;
} PythonReplSource;

static int
open_python_repl_source(PythonReplSource *repl_source) {
  repl_source->source_text =
    (char *)mrbc_raw_alloc(PYTHON_REPL_SOURCE_INITIAL_BYTE_CAPACITY);

  if (repl_source->source_text == NULL)
    return 0;

  repl_source->source_text[0] = '\0';
  repl_source->source_byte_count = 0;

  repl_source->source_byte_capacity =
    PYTHON_REPL_SOURCE_INITIAL_BYTE_CAPACITY;

  return 1;
}

static void
close_python_repl_source(PythonReplSource *repl_source) {
  mrbc_raw_free(repl_source->source_text);
}

static void
reset_python_repl_source(PythonReplSource *repl_source) {
  repl_source->source_text[0] = '\0';
  repl_source->source_byte_count = 0;
}

static int
append_python_repl_source_line(
  PythonReplSource *repl_source,
  const char *input_line,
  int input_byte_count
) {

  int separator_byte_count = repl_source->source_byte_count > 0 ? 1 : 0;

  int required_byte_capacity =
    repl_source->source_byte_count +
    separator_byte_count +
    input_byte_count +
    1;

  if (
    !ensure_byte_capacity(
      &repl_source->source_text,
      &repl_source->source_byte_capacity,
      required_byte_capacity
    )
  )
    return 0;

  if (separator_byte_count > 0) {
    repl_source->source_text[repl_source->source_byte_count] = '\n';

    repl_source->source_byte_count++;
  }

  memcpy(
    repl_source->source_text + repl_source->source_byte_count,
    input_line,
    input_byte_count
  );

  repl_source->source_byte_count += input_byte_count;
  repl_source->source_text[repl_source->source_byte_count] = '\0';

  return 1;
}

static void
run_python_repl(void) {
  int micropython_stack_top;
  PythonReplSource repl_source;

  area512_console_reset();

  if (!open_python_repl_source(&repl_source)) {
    write_console_line(OUT_OF_MEMORY_MESSAGE);

    return;
  }

  int open_runtime_result =
    area512_micropython_open_repl_runtime(&micropython_stack_top);

  if (open_runtime_result != 0) {
    close_python_repl_source(&repl_source);
    write_console_line(OUT_OF_MEMORY_MESSAGE);

    return;
  }

  for (;;) {
    const char *prompt =
      repl_source.source_byte_count > 0
        ? PYTHON_REPL_CONTINUATION_PROMPT
        : PYTHON_REPL_PRIMARY_PROMPT;

    ReplLine repl_line;

    if (!open_repl_line(&repl_line)) {
      write_console_line(OUT_OF_MEMORY_MESSAGE);

      break;
    }

    int input_status = read_repl_input_line(prompt, &repl_line);

    if (input_status == REPL_INPUT_ESCAPED) {
      close_repl_line(&repl_line);

      break;
    }

    if (input_status == REPL_INPUT_OUT_OF_MEMORY) {
      close_repl_line(&repl_line);
      write_console_line(OUT_OF_MEMORY_MESSAGE);

      break;
    }

    if (input_status == REPL_INPUT_INTERRUPTED) {
      close_repl_line(&repl_line);
      reset_python_repl_source(&repl_source);

      continue;
    }

    echo_repl_input_line(prompt, &repl_line);

    if (
      !append_python_repl_source_line(
        &repl_source,
        repl_line.input_line,
        repl_line.input_byte_count
      )
    ) {

      close_repl_line(&repl_line);
      write_console_line(OUT_OF_MEMORY_MESSAGE);

      break;
    }

    close_repl_line(&repl_line);

    if (repl_source.source_byte_count == 0)
      continue;

    if (area512_micropython_repl_source_is_incomplete(repl_source.source_text))
      continue;

    area512_micropython_exec_repl_source(repl_source.source_text);

    reset_python_repl_source(&repl_source);
  }

  area512_micropython_close_repl_runtime();
  close_python_repl_source(&repl_source);
}

static void
c_python_repl_start(
  mrbc_vm *virtual_machine,
  mrbc_value *ruby_method_arguments,
  int ruby_method_argument_count
) {

  (void)virtual_machine;
  (void)ruby_method_argument_count;

  run_python_repl();

  mrbc_decref(ruby_method_arguments);
  mrbc_set_nil(ruby_method_arguments);
}

void
define_python_repl_class(mrbc_vm *virtual_machine) {
  mrbc_class *class_Area512 =
    mrbc_define_class(virtual_machine, "Area512", mrbc_class_object);

  mrbc_class *class_PythonRepl =
    mrbc_define_class_under(
      virtual_machine,
      class_Area512,
      "PythonRepl",
      mrbc_class_object
    );

  mrbc_define_method(
    virtual_machine,
    class_PythonRepl,
    "start",
    c_python_repl_start
  );
}

#endif
