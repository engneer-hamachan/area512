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

static char *
build_sd_path(mrbc_vm *virtual_machine, const char *path) {
  size_t path_length = strlen(path);
  bool absolute = path_length > 0 && path[0] == '/';

  size_t full_path_length =
    strlen(SD_BASE_PATH) + path_length + (absolute ? 0 : 1);

  char *full_path =
    (char *)mrbc_alloc(virtual_machine, (unsigned int)(full_path_length + 1));

  if (full_path == NULL) {
    return NULL;
  }

  strcpy(full_path, SD_BASE_PATH);

  if (!absolute) {
    strcat(full_path, "/");
  }

  strcat(full_path, path);

  return full_path;
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
c_sdfat__is_exist(mrbc_vm *virtual_machine, mrbc_value v[], int argument_count) {
  const char *path = (const char *)GET_STRING_ARG(1);
  char *full_path = build_sd_path(virtual_machine, path);

  if (full_path == NULL) {
    SET_FALSE_RETURN();
    return;
  }

  struct stat stat_buffer;

  bool exists = stat(full_path, &stat_buffer) == 0;

  mrbc_free(virtual_machine, full_path);

  SET_BOOL_RETURN(exists);
}

static void
c_sdfat__read(mrbc_vm *virtual_machine, mrbc_value v[], int argument_count) {
  const char *path = (const char *)GET_STRING_ARG(1);
  char *full_path = build_sd_path(virtual_machine, path);

  if (full_path == NULL) {
    raise_no_memory(virtual_machine);
    return;
  }

  FILE *file_pointer = fopen(full_path, "rb");

  if (file_pointer == NULL) {
    mrbc_free(virtual_machine, full_path);
    mrbc_raise(virtual_machine, MRBC_CLASS(RuntimeError), strerror(errno));

    return;
  }

  struct stat stat_buffer;

  if (stat(full_path, &stat_buffer) != 0) {
    mrbc_free(virtual_machine, full_path);
    fclose(file_pointer);
    mrbc_raise(virtual_machine, MRBC_CLASS(RuntimeError), strerror(errno));

    return;
  }

  mrbc_free(virtual_machine, full_path);

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

  mrbc_value result =
    mrbc_string_new(
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
  const char *path = (const char *)GET_STRING_ARG(1);
  mrbc_value content = v[2];

  char *full_path = build_sd_path(virtual_machine, path);
  if (full_path == NULL) {
    raise_no_memory(virtual_machine);
    return;
  }

  FILE *file_pointer = fopen(full_path, "wb");

  mrbc_free(virtual_machine, full_path);

  if (file_pointer == NULL) {
    mrbc_raise(virtual_machine, MRBC_CLASS(RuntimeError), strerror(errno));
    return;
  }

  size_t bytes_written =
    fwrite(
      content.string->data,
      1,
      (size_t)content.string->size,
      file_pointer
    );

  fflush(file_pointer);
  fclose(file_pointer);

  SET_INT_RETURN((mrbc_int_t)bytes_written);
}

void
mrbc_area512_sdfat_init(mrbc_vm *virtual_machine) {
  mrbc_class *module_SD = mrbc_define_module(virtual_machine, "SD");

  mrbc_define_method(virtual_machine, module_SD, "mount", c_sdfat__mount);
  mrbc_define_method(virtual_machine, module_SD, "unmount", c_sdfat__unmount);
  mrbc_define_method(virtual_machine, module_SD, "exist?", c_sdfat__is_exist);
  mrbc_define_method(virtual_machine, module_SD, "read", c_sdfat__read);
  mrbc_define_method(virtual_machine, module_SD, "write", c_sdfat__write);
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
