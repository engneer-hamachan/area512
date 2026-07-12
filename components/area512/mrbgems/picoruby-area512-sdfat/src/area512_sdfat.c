// microSD (FAT) gem: simple whole-file access for mrubyc.
// Hardware init is delegated to area512_sd.c.

#include <stdint.h>

#if defined(PICORB_VM_MRUBYC)

#include "area512_hal.h"
#include <errno.h>
#include <mrubyc.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#define SD_BASE_PATH "/sdcard"

static bool
resolve_path_argument(
  mrbc_vm *virtual_machine,
  mrbc_value v[],
  char *buffer,
  size_t buffer_size
) {

  const char *path = (const char *)GET_STRING_ARG(1);

  if (area512_resolve_data_path(path, buffer, buffer_size) != 0) {
    mrbc_raise(virtual_machine, MRBC_CLASS(RuntimeError), "invalid path");
    return false;
  }

  return true;
}

static void
raise_no_memory(mrbc_vm *virtual_machine) {
  mrbc_raise(virtual_machine, MRBC_CLASS(RuntimeError), "No memory");
}

static void
c_sdfat__mount(mrbc_vm *virtual_machine, mrbc_value v[], int argument_count) {
  if (area512_sd_mount(SD_BASE_PATH) < 0) {
    mrbc_raise(virtual_machine, MRBC_CLASS(RuntimeError), "SD card not found");
    return;
  }

  SET_TRUE_RETURN();
}

static void
c_sdfat__unmount(mrbc_vm *virtual_machine, mrbc_value v[], int argument_count) {
  area512_sd_unmount();

  SET_NIL_RETURN();
}

static void
c_sdfat__is_exist(
  mrbc_vm *virtual_machine,
  mrbc_value v[],
  int argument_count
) {
  const char *path = (const char *)GET_STRING_ARG(1);
  char full_path[AREA512_PATH_MAX];

  if (area512_resolve_data_path(path, full_path, sizeof full_path) != 0) {
    SET_FALSE_RETURN();
    return;
  }

  struct stat stat_buffer;

  SET_BOOL_RETURN(stat(full_path, &stat_buffer) == 0);
}

static void
c_sdfat__mkdir(mrbc_vm *virtual_machine, mrbc_value v[], int argument_count) {
  char full_path[AREA512_PATH_MAX];

  if (!resolve_path_argument(virtual_machine, v, full_path, sizeof full_path))
    return;

  if (mkdir(full_path, 0777) != 0 && errno != EEXIST) {
    mrbc_raise(virtual_machine, MRBC_CLASS(RuntimeError), strerror(errno));
    return;
  }

  SET_TRUE_RETURN();
}

static void
c_sdfat__read(mrbc_vm *virtual_machine, mrbc_value v[], int argument_count) {
  char full_path[AREA512_PATH_MAX];

  if (!resolve_path_argument(virtual_machine, v, full_path, sizeof full_path))
    return;

  FILE *file_pointer = fopen(full_path, "rb");

  if (file_pointer == NULL) {
    mrbc_raise(virtual_machine, MRBC_CLASS(RuntimeError), strerror(errno));
    return;
  }

  struct stat stat_buffer;

  if (stat(full_path, &stat_buffer) != 0) {
    fclose(file_pointer);
    mrbc_raise(virtual_machine, MRBC_CLASS(RuntimeError), strerror(errno));

    return;
  }

  long file_size = (long)stat_buffer.st_size;
  char *buffer = NULL;

  if (file_size > 0) {
    buffer = (char *)mrbc_alloc(virtual_machine, (unsigned int)file_size);

    if (buffer == NULL) {
      fclose(file_pointer);
      raise_no_memory(virtual_machine);

      return;
    }
  }

  size_t bytes_read = 0;

  if (file_size > 0) {
    bytes_read = fread(buffer, 1, (size_t)file_size, file_pointer);
  }

  fclose(file_pointer);

  if (bytes_read != (size_t)file_size) {
    if (buffer != NULL) {
      mrbc_free(virtual_machine, buffer);
    }

    mrbc_raise(virtual_machine, MRBC_CLASS(RuntimeError), "read failed");

    return;
  }

  mrbc_value result = mrbc_string_new(
    virtual_machine,
    file_size > 0 ? (const void *)buffer : "",
    (uint32_t)file_size
  );

  if (buffer != NULL) {
    mrbc_free(virtual_machine, buffer);
  }

  SET_RETURN(result);
}

static void
c_sdfat__write(mrbc_vm *virtual_machine, mrbc_value v[], int argument_count) {
  mrbc_value content = v[2];
  char full_path[AREA512_PATH_MAX];

  if (!resolve_path_argument(virtual_machine, v, full_path, sizeof full_path))
    return;

  char temporary_path[AREA512_PATH_MAX + sizeof(".tmp")];

  strcpy(temporary_path, full_path);
  strcat(temporary_path, ".tmp");

  FILE *file_pointer = fopen(temporary_path, "wb");

  if (file_pointer == NULL) {
    mrbc_raise(virtual_machine, MRBC_CLASS(RuntimeError), strerror(errno));
    return;
  }

  size_t bytes_written =
    fwrite(content.string->data, 1, (size_t)content.string->size, file_pointer);

  bool closed = fclose(file_pointer) == 0;

  if (!closed || bytes_written != (size_t)content.string->size) {
    remove(temporary_path);
    mrbc_raise(virtual_machine, MRBC_CLASS(RuntimeError), "write failed");

    return;
  }

  // FAT's rename can't replace an existing file, so remove the target first;
  // the fully written .tmp file survives a power cut in between.
  if (rename(temporary_path, full_path) != 0) {
    if (remove(full_path) != 0 || rename(temporary_path, full_path) != 0) {
      remove(temporary_path);
      mrbc_raise(virtual_machine, MRBC_CLASS(RuntimeError), strerror(errno));

      return;
    }
  }

  SET_INT_RETURN((mrbc_int_t)bytes_written);
}

static void
c_sdfat__restore_seed(
  mrbc_vm *virtual_machine,
  mrbc_value v[],
  int argument_count
) {

  if (area512_seed_restore() != 0) {
    mrbc_raise(
      virtual_machine,
      MRBC_CLASS(RuntimeError),
      "seed restore failed"
    );

    return;
  }

  SET_TRUE_RETURN();
}

void mrbc_area512_fs_register(mrbc_vm *virtual_machine);

void
mrbc_area512_sdfat_init(mrbc_vm *virtual_machine) {
  mrbc_class *module_SD = mrbc_define_module(virtual_machine, "SD");

  mrbc_define_method(virtual_machine, module_SD, "mount", c_sdfat__mount);
  mrbc_define_method(virtual_machine, module_SD, "unmount", c_sdfat__unmount);
  mrbc_define_method(virtual_machine, module_SD, "exist?", c_sdfat__is_exist);
  mrbc_define_method(virtual_machine, module_SD, "mkdir", c_sdfat__mkdir);
  mrbc_define_method(virtual_machine, module_SD, "read", c_sdfat__read);
  mrbc_define_method(virtual_machine, module_SD, "write", c_sdfat__write);

  mrbc_define_method(
    virtual_machine,
    module_SD,
    "restore_seed",
    c_sdfat__restore_seed
  );

  mrbc_area512_fs_register(virtual_machine);
}

#else // PICORB_VM_MRUBY: unused in this build; empty stubs

#include <mruby.h>

void
mrb_picoruby_area512_sdfat_gem_init(mrb_state *state) {
  (void)state;
}

void
mrb_picoruby_area512_sdfat_gem_final(mrb_state *state) {
  (void)state;
}

#endif
