#pragma once

int area512_micropython_compile_python_source_file(
  const char *python_source_path,
  const char *python_bytecode_path
);

int area512_micropython_run_python_bytecode_file(
  const char *python_bytecode_path
);

int area512_micropython_run_python_manifest(
  const char *directory_path,
  const char *manifest_path
);

int area512_micropython_open_repl_runtime(
  void *micropython_stack_top
);

void area512_micropython_exec_repl_source(
  const char *python_source
);

int area512_micropython_repl_source_is_incomplete(
  const char *python_source
);

void area512_micropython_close_repl_runtime(void);
