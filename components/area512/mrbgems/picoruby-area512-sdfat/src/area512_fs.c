// Minimal libc-backed File/Dir for main_task.rb, require.rb and Sandbox;
// Ruby-visible paths are rooted at AREA512_DATA_ROOT ("/" == the data root).

#include <stdint.h>

#if defined(PICORB_VM_MRUBYC)

#include "area512_hal.h"

#include <dirent.h>
#include <errno.h>
#include <mrubyc.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static void
raise_errno(mrbc_vm *virtual_machine) {
  mrbc_raise(virtual_machine, MRBC_CLASS(RuntimeError), strerror(errno));
}

static bool
resolve_path_argument(
  mrbc_vm *virtual_machine,
  mrbc_value v[],
  int argument_index,
  char *buffer,
  size_t buffer_size
) {

  const char *path = (const char *)GET_STRING_ARG(argument_index);

  if (area512_resolve_data_path(path, buffer, buffer_size) != 0) {
    mrbc_raise(virtual_machine, MRBC_CLASS(RuntimeError), "invalid path");
    return false;
  }

  return true;
}

static bool
stat_path(mrbc_value v[], struct stat *stat_buffer) {
  const char *path = (const char *)GET_STRING_ARG(1);
  char full_path[AREA512_PATH_MAX];

  if (area512_resolve_data_path(path, full_path, sizeof full_path) != 0)
    return false;

  return stat(full_path, stat_buffer) == 0;
}

static void
c_file_is_exist(mrbc_vm *virtual_machine, mrbc_value v[], int argument_count) {
  struct stat stat_buffer;

  SET_BOOL_RETURN(stat_path(v, &stat_buffer));
}

static void
c_file_is_file(mrbc_vm *virtual_machine, mrbc_value v[], int argument_count) {
  struct stat stat_buffer;

  SET_BOOL_RETURN(stat_path(v, &stat_buffer) && S_ISREG(stat_buffer.st_mode));
}

static void
c_file_is_directory(
  mrbc_vm *virtual_machine,
  mrbc_value v[],
  int argument_count
) {

  struct stat stat_buffer;

  SET_BOOL_RETURN(stat_path(v, &stat_buffer) && S_ISDIR(stat_buffer.st_mode));
}

static void
c_file_unlink(mrbc_vm *virtual_machine, mrbc_value v[], int argument_count) {
  char full_path[AREA512_PATH_MAX];

  if (!resolve_path_argument(virtual_machine, v, 1, full_path, sizeof full_path))
    return;

  if (unlink(full_path) != 0) {
    raise_errno(virtual_machine);
    return;
  }

  SET_INT_RETURN(1);
}

static void
c_file_rename(mrbc_vm *virtual_machine, mrbc_value v[], int argument_count) {
  char source_path[AREA512_PATH_MAX];
  char destination_path[AREA512_PATH_MAX];

  if (!resolve_path_argument(virtual_machine, v, 1, source_path, sizeof source_path))
    return;

  if (
    !resolve_path_argument(
      virtual_machine,
      v,
      2,
      destination_path,
      sizeof destination_path
    )
  )
    return;

  if (rename(source_path, destination_path) != 0) {
    raise_errno(virtual_machine);
    return;
  }

  SET_INT_RETURN(0);
}

static void
c_file_new(mrbc_vm *virtual_machine, mrbc_value v[], int argument_count) {
  const char *mode = argument_count >= 2 ? (const char *)GET_STRING_ARG(2) : "r";
  char full_path[AREA512_PATH_MAX];

  if (!resolve_path_argument(virtual_machine, v, 1, full_path, sizeof full_path))
    return;

  FILE *file = fopen(full_path, strcmp(mode, "w") == 0 ? "wb" : "rb");

  if (file == NULL) {
    raise_errno(virtual_machine);
    return;
  }

  mrbc_value instance =
    mrbc_instance_new(virtual_machine, v->cls, sizeof(FILE *));

  *(FILE **)instance.instance->data = file;

  SET_RETURN(instance);
}

static void
c_file_read(mrbc_vm *virtual_machine, mrbc_value v[], int argument_count) {
  FILE *file = *(FILE **)v->instance->data;

  if (file == NULL) {
    mrbc_raise(virtual_machine, MRBC_CLASS(RuntimeError), "closed file");
    return;
  }

  long start_position = ftell(file);

  if (start_position < 0 || fseek(file, 0, SEEK_END) != 0) {
    raise_errno(virtual_machine);
    return;
  }

  long end_position = ftell(file);

  if (end_position < 0 || fseek(file, start_position, SEEK_SET) != 0) {
    raise_errno(virtual_machine);
    return;
  }

  size_t size = (size_t)(end_position - start_position);

  if (size == 0) {
    SET_RETURN(mrbc_string_new(virtual_machine, "", 0));
    return;
  }

  char *buffer = (char *)mrbc_alloc(virtual_machine, (unsigned int)size);

  if (buffer == NULL) {
    mrbc_raise(virtual_machine, MRBC_CLASS(RuntimeError), "No memory");
    return;
  }

  size_t bytes_read = fread(buffer, 1, size, file);

  if (bytes_read != size) {
    mrbc_free(virtual_machine, buffer);
    mrbc_raise(virtual_machine, MRBC_CLASS(RuntimeError), "read failed");

    return;
  }

  mrbc_value result =
    mrbc_string_new(virtual_machine, buffer, (uint32_t)size);

  mrbc_free(virtual_machine, buffer);

  SET_RETURN(result);
}

static void
c_file_close(mrbc_vm *virtual_machine, mrbc_value v[], int argument_count) {
  FILE *file = *(FILE **)v->instance->data;

  if (file != NULL) {
    fclose(file);

    *(FILE **)v->instance->data = NULL;
  }

  SET_NIL_RETURN();
}

static void
close_file_on_gc(mrbc_value *self) {
  FILE *file = *(FILE **)self->instance->data;

  if (file != NULL) {
    fclose(file);
  }
}

static void
c_dir_is_exist(mrbc_vm *virtual_machine, mrbc_value v[], int argument_count) {
  struct stat stat_buffer;

  SET_BOOL_RETURN(stat_path(v, &stat_buffer) && S_ISDIR(stat_buffer.st_mode));
}

static void
c_dir_mkdir(mrbc_vm *virtual_machine, mrbc_value v[], int argument_count) {
  char full_path[AREA512_PATH_MAX];

  if (!resolve_path_argument(virtual_machine, v, 1, full_path, sizeof full_path))
    return;

  if (mkdir(full_path, 0777) != 0) {
    raise_errno(virtual_machine);
    return;
  }

  SET_INT_RETURN(0);
}

static void
c_dir_rmdir(mrbc_vm *virtual_machine, mrbc_value v[], int argument_count) {
  char full_path[AREA512_PATH_MAX];

  if (!resolve_path_argument(virtual_machine, v, 1, full_path, sizeof full_path))
    return;

  if (rmdir(full_path) != 0) {
    raise_errno(virtual_machine);
    return;
  }

  SET_INT_RETURN(0);
}

static void
c_dir_new(mrbc_vm *virtual_machine, mrbc_value v[], int argument_count) {
  char full_path[AREA512_PATH_MAX];

  if (!resolve_path_argument(virtual_machine, v, 1, full_path, sizeof full_path))
    return;

  DIR *directory = opendir(full_path);

  if (directory == NULL) {
    raise_errno(virtual_machine);
    return;
  }

  mrbc_value instance =
    mrbc_instance_new(virtual_machine, v->cls, sizeof(DIR *));

  *(DIR **)instance.instance->data = directory;

  SET_RETURN(instance);
}

static void
c_dir_read(mrbc_vm *virtual_machine, mrbc_value v[], int argument_count) {
  DIR *directory = *(DIR **)v->instance->data;

  if (directory == NULL) {
    mrbc_raise(virtual_machine, MRBC_CLASS(RuntimeError), "closed directory");
    return;
  }

  struct dirent *entry = readdir(directory);

  if (entry == NULL) {
    SET_NIL_RETURN();
    return;
  }

  mrbc_value result = mrbc_string_new_cstr(virtual_machine, entry->d_name);

  SET_RETURN(result);
}

static void
c_dir_close(mrbc_vm *virtual_machine, mrbc_value v[], int argument_count) {
  DIR *directory = *(DIR **)v->instance->data;

  if (directory != NULL) {
    closedir(directory);

    *(DIR **)v->instance->data = NULL;
  }

  SET_NIL_RETURN();
}

static void
close_dir_on_gc(mrbc_value *self) {
  DIR *directory = *(DIR **)self->instance->data;

  if (directory != NULL) {
    closedir(directory);
  }
}

void
mrbc_area512_fs_register(mrbc_vm *virtual_machine) {
  mrbc_class *class_File =
    mrbc_define_class(virtual_machine, "File", mrbc_class_object);

  mrbc_define_destructor(class_File, close_file_on_gc);
  mrbc_define_method(virtual_machine, class_File, "new", c_file_new);
  mrbc_define_method(virtual_machine, class_File, "exist?", c_file_is_exist);
  mrbc_define_method(virtual_machine, class_File, "file?", c_file_is_file);

  mrbc_define_method(
    virtual_machine,
    class_File,
    "directory?",
    c_file_is_directory
  );

  mrbc_define_method(virtual_machine, class_File, "unlink", c_file_unlink);
  mrbc_define_method(virtual_machine, class_File, "rename", c_file_rename);
  mrbc_define_method(virtual_machine, class_File, "read", c_file_read);
  mrbc_define_method(virtual_machine, class_File, "close", c_file_close);

  mrbc_class *class_Dir =
    mrbc_define_class(virtual_machine, "Dir", mrbc_class_object);

  mrbc_define_destructor(class_Dir, close_dir_on_gc);
  mrbc_define_method(virtual_machine, class_Dir, "new", c_dir_new);
  mrbc_define_method(virtual_machine, class_Dir, "exist?", c_dir_is_exist);
  mrbc_define_method(virtual_machine, class_Dir, "mkdir", c_dir_mkdir);
  mrbc_define_method(virtual_machine, class_Dir, "rmdir", c_dir_rmdir);
  mrbc_define_method(virtual_machine, class_Dir, "read", c_dir_read);
  mrbc_define_method(virtual_machine, class_Dir, "close", c_dir_close);
}

#endif
