#if defined(PICORB_VM_MRUBYC)

#include "area512_hal.h"
#include "area512_repl.h"

#include <string.h>

#include <mrubyc.h>

static void
c_repl_read_line(mrbc_vm *virtual_machine, mrbc_value *v, int argument_count) {
  (void)argument_count;

  const char *prompt = (const char *)GET_STRING_ARG(1);

  ReplLine repl_line;

  if (!open_repl_line(&repl_line)) {
    SET_RETURN(mrbc_symbol_new(virtual_machine, "out_of_memory"));
    return;
  }

  int input_status = read_repl_input_line(prompt, &repl_line);

  if (input_status == REPL_INPUT_SUBMITTED) {
    area512_console_write(prompt, strlen(prompt));
    area512_console_write(repl_line.input_line, repl_line.input_byte_count);
    area512_console_write("\n", 1);

    mrbc_value result =
      mrbc_string_new_cstr(virtual_machine, repl_line.input_line);

    close_repl_line(&repl_line);

    SET_RETURN(result);

    return;
  }

  close_repl_line(&repl_line);

  if (input_status == REPL_INPUT_ESCAPED) {
    SET_RETURN(mrbc_symbol_new(virtual_machine, "escape"));
    return;
  }

  if (input_status == REPL_INPUT_INTERRUPTED) {
    SET_RETURN(mrbc_symbol_new(virtual_machine, "interrupt"));
    return;
  }

  SET_RETURN(mrbc_symbol_new(virtual_machine, "out_of_memory"));
}

static void
c_repl_reset(mrbc_vm *virtual_machine, mrbc_value *v, int argument_count) {
  (void)virtual_machine;
  (void)v;
  (void)argument_count;

  area512_console_reset();
}

static void
c_repl_write_line(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int argument_count
) {

  (void)argument_count;

  if (v[1].tt != MRBC_TT_STRING) {
    mrbc_raise(virtual_machine, MRBC_CLASS(TypeError), "expected String");
    return;
  }

  const char *text = (const char *)v[1].string->data;

  area512_console_write(text, strlen(text));
  area512_console_write("\n", 1);

  SET_NIL_RETURN();
}

void
mrbc_area512_repl_init(mrbc_vm *virtual_machine) {
  mrbc_class *class_Area512 =
    mrbc_define_class(virtual_machine, "Area512", mrbc_class_object);

  mrbc_class *class_Repl =
    mrbc_define_class_under(
      virtual_machine,
      class_Area512,
      "Repl",
      mrbc_class_object
    );

  mrbc_define_method(
    virtual_machine,
    class_Repl,
    "read_line",
    c_repl_read_line
  );

  mrbc_define_method(
    virtual_machine,
    class_Repl,
    "reset",
    c_repl_reset
  );

  mrbc_define_method(
    virtual_machine,
    class_Repl,
    "write_line",
    c_repl_write_line
  );
}

#endif
