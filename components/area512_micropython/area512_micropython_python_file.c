#include "area512_hal.h"
#include "area512_micropython.h"
#include "area512_micropython_gc_heap.h"
#include "port/micropython_embed.h"
#include "py/compile.h"
#include "py/gc.h"
#include "py/lexer.h"
#include "py/mpprint.h"
#include "py/persistentcode.h"
#include "py/runtime.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  FILE *bytecode_file;
  int write_failed;
} PythonBytecodeFileWriter;

static char *
read_whole_file(const char *path, size_t *file_byte_length) {
  char resolved_path[AREA512_PATH_MAX];

  if (area512_resolve_data_path(path, resolved_path, sizeof resolved_path) != 0)
    return NULL;

  FILE *file = fopen(resolved_path, "rb");

  if (!file)
    return NULL;

  long file_byte_size = -1;

  if (fseek(file, 0, SEEK_END) == 0)
    file_byte_size = ftell(file);

  if (file_byte_size < 0 || fseek(file, 0, SEEK_SET) != 0) {
    fclose(file);

    return NULL;
  }

  char *file_bytes = malloc((size_t)file_byte_size + 1);

  if (!file_bytes) {
    fclose(file);

    return NULL;
  }

  size_t read_byte_count = fread(file_bytes, 1, (size_t)file_byte_size, file);

  fclose(file);

  if (read_byte_count != (size_t)file_byte_size) {
    free(file_bytes);

    return NULL;
  }

  file_bytes[read_byte_count] = '\0';
  *file_byte_length = read_byte_count;

  return file_bytes;
}

static void
write_python_bytecode(
  void *writer_pointer,
  const char *bytecode_bytes,
  size_t bytecode_byte_length
) {

  PythonBytecodeFileWriter *writer = (PythonBytecodeFileWriter *)writer_pointer;

  size_t written_byte_count =
    fwrite(bytecode_bytes, 1, bytecode_byte_length, writer->bytecode_file);

  if (written_byte_count != bytecode_byte_length)
    writer->write_failed = 1;
}

static int
compile_python_source_to_bytecode(
  const char *python_source_path,
  const char *python_source_bytes,
  size_t python_source_byte_length,
  FILE *bytecode_file
) {

  PythonBytecodeFileWriter writer = {bytecode_file, 0};
  mp_print_t bytecode_print = {&writer, write_python_bytecode};

  nlr_buf_t nlr_buffer;

  if (nlr_push(&nlr_buffer) != 0) {
    mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr_buffer.ret_val);

    return -1;
  }

  qstr python_source_name = qstr_from_str(python_source_path);

  mp_lexer_t *lexer =
    mp_lexer_new_from_str_len(
      python_source_name,
      python_source_bytes,
      python_source_byte_length,
      0
    );

  mp_parse_tree_t parse_tree = mp_parse(lexer, MP_PARSE_FILE_INPUT);

  mp_compiled_module_t compiled_module;

  compiled_module.context = m_new_obj(mp_module_context_t);
  compiled_module.arch_flags = 0;

  mp_compile_to_raw_code(
    &parse_tree,
    python_source_name,
    false,
    &compiled_module
  );

  mp_raw_code_save(&compiled_module, &bytecode_print);

  nlr_pop();

  return writer.write_failed ? -1 : 0;
}

int
area512_micropython_compile_python_source_file(
  const char *python_source_path,
  const char *python_bytecode_path
) {

  size_t python_source_byte_length = 0;

  char *python_source_bytes =
    read_whole_file(
      python_source_path,
      &python_source_byte_length
    );

  if (!python_source_bytes)
    return -1;

  char resolved_python_bytecode_path[AREA512_PATH_MAX];

  if (
    area512_resolve_data_path(
      python_bytecode_path,
      resolved_python_bytecode_path,
      sizeof resolved_python_bytecode_path
    ) != 0
  ) {

    free(python_source_bytes);

    return -1;
  }

  FILE *python_bytecode_file =
    fopen(resolved_python_bytecode_path, "wb");

  if (!python_bytecode_file) {
    free(python_source_bytes);

    return -1;
  }

  size_t micropython_gc_heap_byte_size = 0;
  void *micropython_gc_heap =
    area512_micropython_allocate_gc_heap(&micropython_gc_heap_byte_size);

  if (!micropython_gc_heap) {
    fclose(python_bytecode_file);
    remove(resolved_python_bytecode_path);
    free(python_source_bytes);

    return -1;
  }

  int micropython_stack_top;

  mp_embed_init(
    micropython_gc_heap,
    micropython_gc_heap_byte_size,
    &micropython_stack_top
  );

  int compile_result =
    compile_python_source_to_bytecode(
      python_source_path,
      python_source_bytes,
      python_source_byte_length,
      python_bytecode_file
    );

  gc_sweep_all();
  mp_embed_deinit();
  area512_micropython_free_gc_heap_split_areas();

  free(micropython_gc_heap);
  free(python_source_bytes);

  if (fclose(python_bytecode_file) != 0)
    compile_result = -1;

  if (compile_result != 0)
    remove(resolved_python_bytecode_path);

  return compile_result;
}

static int
exec_python_bytecode(
  const uint8_t *python_bytecode_bytes,
  size_t python_bytecode_byte_length
) {

  nlr_buf_t nlr_buffer;

  if (nlr_push(&nlr_buffer) != 0) {
    mp_obj_print_exception(
      &mp_plat_print,
      (mp_obj_t)nlr_buffer.ret_val
    );

    return -1;
  }

  mp_module_context_t *module_context = m_new_obj(mp_module_context_t);
  module_context->module.globals = mp_globals_get();

  mp_compiled_module_t compiled_module;

  compiled_module.context = module_context;

  mp_raw_code_load_mem(
    python_bytecode_bytes,
    python_bytecode_byte_length,
    &compiled_module
  );

  mp_obj_t module_function =
    mp_make_function_from_proto_fun(
      compiled_module.rc,
      module_context,
      MP_OBJ_NULL
    );

  mp_call_function_0(module_function);

  nlr_pop();

  return 0;
}

static int
run_manifest_entry(const char *directory_path, const char *entry_name) {
  char entry_path[AREA512_PATH_MAX];

  if (entry_name[0] == '/')
    snprintf(entry_path, sizeof entry_path, "%s", entry_name);
  else
    snprintf(
      entry_path,
      sizeof entry_path,
      "%s/%s",
      directory_path,
      entry_name
    );

  size_t python_bytecode_byte_length = 0;

  char *python_bytecode_bytes =
    read_whole_file(
      entry_path,
      &python_bytecode_byte_length
    );

  if (!python_bytecode_bytes)
    return -1;

  int exec_result =
    exec_python_bytecode(
      (const uint8_t *)python_bytecode_bytes,
      python_bytecode_byte_length
    );

  free(python_bytecode_bytes);

  return exec_result;
}

static int
run_manifest_entries(const char *directory_path, char *manifest_text) {
  char *line_start = manifest_text;

  while (*line_start) {
    char *line_end = line_start;

    while (*line_end && *line_end != '\n')
      line_end++;

    char *next_line_start = *line_end ? line_end + 1 : line_end;

    while (
      line_end > line_start &&
      (line_end[-1] == '\r' || line_end[-1] == ' ' || line_end[-1] == '\t')
    ) {

      line_end--;
    }

    *line_end = '\0';

    while (*line_start == ' ' || *line_start == '\t')
      line_start++;

    if (*line_start && *line_start != '#') {
      int entry_result =
        run_manifest_entry(directory_path, line_start);

      if (entry_result != 0)
        return entry_result;
    }

    line_start = next_line_start;
  }

  return 0;
}

int
area512_micropython_run_python_bytecode_file(const char *python_bytecode_path) {
  size_t python_bytecode_byte_length = 0;

  char *python_bytecode_bytes =
    read_whole_file(
      python_bytecode_path,
      &python_bytecode_byte_length
    );

  if (!python_bytecode_bytes)
    return -1;

  size_t micropython_gc_heap_byte_size = 0;

  void *micropython_gc_heap =
    area512_micropython_allocate_gc_heap(&micropython_gc_heap_byte_size);

  if (!micropython_gc_heap) {
    free(python_bytecode_bytes);

    return -1;
  }

  int micropython_stack_top;

  mp_embed_init(
    micropython_gc_heap,
    micropython_gc_heap_byte_size,
    &micropython_stack_top
  );

  int run_result =
    exec_python_bytecode(
      (const uint8_t *)python_bytecode_bytes,
      python_bytecode_byte_length
    );

  gc_sweep_all();
  mp_embed_deinit();
  area512_micropython_free_gc_heap_split_areas();
  free(micropython_gc_heap);
  free(python_bytecode_bytes);

  return run_result;
}

int
area512_micropython_run_python_manifest(
  const char *directory_path,
  const char *manifest_path
) {

  size_t manifest_byte_length = 0;

  char *manifest_text =
    read_whole_file(
      manifest_path,
      &manifest_byte_length
    );

  if (!manifest_text)
    return -1;

  size_t micropython_gc_heap_byte_size = 0;

  void *micropython_gc_heap =
    area512_micropython_allocate_gc_heap(&micropython_gc_heap_byte_size);

  if (!micropython_gc_heap) {
    free(manifest_text);

    return -1;
  }

  int micropython_stack_top;

  mp_embed_init(
    micropython_gc_heap,
    micropython_gc_heap_byte_size,
    &micropython_stack_top
  );

  int run_result = run_manifest_entries(directory_path, manifest_text);

  gc_sweep_all();
  mp_embed_deinit();
  area512_micropython_free_gc_heap_split_areas();
  free(micropython_gc_heap);
  free(manifest_text);

  return run_result;
}
