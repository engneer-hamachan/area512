// Uses the mruby-compiler2 API directly, not Sandbox: the upstream Sandbox
// leaks heap on every compile and cannot avoid it.

#include <stdint.h>

#if defined(PICORB_VM_MRUBYC)

#include <stddef.h>
#include <string.h>

#include "littlefs.h"
#include <mrc_codegen.h>
#include <mrc_presym.h>
#include <mruby_compiler.h>
#include <mrubyc.h>

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

static bool
write_lfs_file(
  mrbc_vm *virtual_machine,
  const char *path,
  const uint8_t *data,
  size_t size
) {

  int error = littlefs_ensure_mounted();

  mrbc_raise_iff_lfs_error(virtual_machine, error, "lfs_mount");
  if (error < 0)
    return false;

  lfs_file_t file;

  error = lfs_file_open(
    littlefs_get_lfs(),
    &file,
    path,
    LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC
  );

  mrbc_raise_iff_lfs_error(virtual_machine, error, path);
  if (error < 0)
    return false;

  lfs_ssize_t written =
    lfs_file_write(littlefs_get_lfs(), &file, data, (lfs_size_t)size);

  if (written < 0) {
    lfs_file_close(littlefs_get_lfs(), &file);
    mrbc_raise_iff_lfs_error(virtual_machine, (int)written, "lfs_file_write");

    return false;
  }

  error = lfs_file_sync(littlefs_get_lfs(), &file);
  if (error >= 0) {
    error = lfs_file_close(littlefs_get_lfs(), &file);
  } else {
    lfs_file_close(littlefs_get_lfs(), &file);
  }

  mrbc_raise_iff_lfs_error(virtual_machine, error, "lfs_file_close");
  if (error < 0)
    return false;

  uint32_t timestamp = littlefs_get_unixtime();
  lfs_setattr(
    littlefs_get_lfs(),
    path,
    LFS_ATTR_MTIME,
    &timestamp,
    sizeof(timestamp)
  );

  return true;
}

static void
c_area512_compile_file(
  mrbc_vm *virtual_machine,
  mrbc_value *v,
  int argument_count
) {
  (void)argument_count;

  if (v[1].tt != MRBC_TT_STRING || v[2].tt != MRBC_TT_STRING) {
    mrbc_raise(
      virtual_machine,
      MRBC_CLASS(TypeError),
      "expected String source, destination"
    );

    return;
  }

  const char *source_path = (const char *)v[1].string->data;
  const char *destination_path = (const char *)v[2].string->data;

  size_t source_length = 0;

  uint8_t *source = read_lfs_file(virtual_machine, source_path, &source_length);
  if (!source)
    return;

  if (source_length >= 4 && memcmp(source, "RITE", 4) == 0) {
    mrbc_raw_free(source);
    mrbc_raise(virtual_machine, MRBC_CLASS(RuntimeError), "already compiled");

    return;
  }

  mrc_ccontext *context = mrc_ccontext_new(NULL);
  if (!context) {
    mrbc_raw_free(source);
    mrbc_raise(virtual_machine, MRBC_CLASS(RuntimeError), "out of memory");

    return;
  }

  mrc_ccontext_filename(context, source_path);

  // Drive parse -> codegen directly instead of mrc_load_string_cxt: that
  // function never destroys the prism AST (unlike mrc_load_file_cxt), and its
  // mrubyc path swaps cc->options for a scoped pm_options_t it never frees --
  // both leak every compile. Here the AST, parser, ccontext and options all go.
  context->filename_table =
    (mrc_filename_table *)mrbc_raw_alloc(sizeof(mrc_filename_table));

  context->filename_table[0].filename =
    context->filename ? context->filename : source_path;

  context->filename_table[0].start = 0;
  context->filename_table_length = 1;
  context->current_filename_index = 0;

  pm_options_t options;
  memset(&options, 0, sizeof(options));
  pm_string_constant_init(&options.encoding, "UTF-8", 5);

  pm_parser_init(context->p, source, source_length, &options);
  mrc_init_presym(&context->p->constant_pool);
  mrc_node *root = pm_parse(context->p);

  mrc_irep *irep = NULL;
  if (context->p->error_list.size == 0)
    irep = mrc_generate_code(context, root);

  uint8_t *binary = NULL;
  size_t binary_size = 0;
  int dump_result = -1;
  bool wrote = false;

  if (irep) {
    mrc_irep_remove_lv(context, irep);
    dump_result = mrc_dump_irep(context, irep, 0, &binary, &binary_size);

    if (dump_result == MRC_DUMP_OK)
      wrote =
        write_lfs_file(virtual_machine, destination_path, binary, binary_size);
  }

  if (binary)
    mrbc_raw_free(binary);

  if (irep)
    mrc_irep_free(context, irep);

  pm_node_destroy(context->p, root);
  mrc_ccontext_free(context);
  pm_options_free(&options);
  mrbc_raw_free(source);

  if (!irep) {
    mrbc_raise(virtual_machine, MRBC_CLASS(RuntimeError), "compile failed");

    return;
  }

  if (dump_result != MRC_DUMP_OK) {
    mrbc_raise(virtual_machine, MRBC_CLASS(RuntimeError), "dump failed");

    return;
  }

  if (!wrote)
    return; // write_lfs_file already raised

  SET_TRUE_RETURN();
}

void
mrbc_area512_compile_init(mrbc_vm *virtual_machine) {
  mrbc_class *class_Area512 =
    mrbc_define_class(virtual_machine, "Area512", mrbc_class_object);

  mrbc_define_method(
    virtual_machine,
    class_Area512,
    "compile_file",
    c_area512_compile_file
  );
}

#else // PICORB_VM_MRUBY: unused in this build; empty stubs

#include <mruby.h>

void
mrb_picoruby_area512_compile_gem_init(mrb_state *state) {
  (void)state;
}

void
mrb_picoruby_area512_compile_gem_final(mrb_state *state) {
  (void)state;
}

#endif
