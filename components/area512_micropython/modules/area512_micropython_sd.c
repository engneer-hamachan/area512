#include "area512_hal.h"

#include "py/runtime.h"

#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

typedef struct {
  mp_obj_base_t base;
  FILE *file_pointer;
} area512_micropython_file_instance_t;

typedef struct {
  mp_obj_base_t base;
  DIR *directory_pointer;
} area512_micropython_directory_instance_t;

static void
resolve_path_or_raise(
  mp_obj_t path_object,
  char *resolved_path,
  size_t resolved_path_size
) {

  if (area512_resolve_data_path(
        mp_obj_str_get_str(path_object),
        resolved_path,
        resolved_path_size
      ) != 0)
    mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("invalid path"));
}

static bool
stat_path(mp_obj_t path_object, struct stat *stat_buffer) {
  char resolved_path[AREA512_PATH_MAX];

  if (area512_resolve_data_path(
        mp_obj_str_get_str(path_object),
        resolved_path,
        sizeof resolved_path
      ) != 0)
    return false;

  return stat(resolved_path, stat_buffer) == 0;
}

static FILE *
fetch_file_pointer_or_raise(mp_obj_t file_object) {
  area512_micropython_file_instance_t *file_instance =
    MP_OBJ_TO_PTR(file_object);

  if (file_instance->file_pointer == NULL)
    mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("closed file"));

  return file_instance->file_pointer;
}

static DIR *
fetch_directory_pointer_or_raise(mp_obj_t directory_object) {
  area512_micropython_directory_instance_t *directory_instance =
    MP_OBJ_TO_PTR(directory_object);

  if (directory_instance->directory_pointer == NULL)
    mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("closed directory"));

  return directory_instance->directory_pointer;
}

static mp_obj_t
is_existing_path(mp_obj_t path_object) {
  struct stat stat_buffer;

  return mp_obj_new_bool(stat_path(path_object, &stat_buffer));
}
static MP_DEFINE_CONST_FUN_OBJ_1(is_existing_path_callable, is_existing_path);

static mp_obj_t
is_regular_file_path(mp_obj_t path_object) {
  struct stat stat_buffer;

  return mp_obj_new_bool(
    stat_path(path_object, &stat_buffer) && S_ISREG(stat_buffer.st_mode)
  );
}
static MP_DEFINE_CONST_FUN_OBJ_1(
  is_regular_file_path_callable,
  is_regular_file_path
);

static mp_obj_t
is_directory_path(mp_obj_t path_object) {
  struct stat stat_buffer;

  return mp_obj_new_bool(
    stat_path(path_object, &stat_buffer) && S_ISDIR(stat_buffer.st_mode)
  );
}
static MP_DEFINE_CONST_FUN_OBJ_1(is_directory_path_callable, is_directory_path);

static mp_obj_t
mount_sd(void) {
  if (area512_sd_mount("/sdcard") < 0)
    mp_raise_type(&mp_type_OSError);

  return mp_const_true;
}
static MP_DEFINE_CONST_FUN_OBJ_0(mount_sd_callable, mount_sd);

static mp_obj_t
unmount_sd(void) {
  area512_sd_unmount();

  return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(unmount_sd_callable, unmount_sd);

static mp_obj_t
make_sd_directory_if_absent(mp_obj_t path_object) {
  char resolved_path[AREA512_PATH_MAX];

  resolve_path_or_raise(path_object, resolved_path, sizeof resolved_path);

  if (mkdir(resolved_path, 0777) != 0 && errno != EEXIST)
    mp_raise_OSError(errno);

  return mp_const_true;
}
static MP_DEFINE_CONST_FUN_OBJ_1(
  make_sd_directory_if_absent_callable,
  make_sd_directory_if_absent
);

static mp_obj_t
read_whole_sd_file(mp_obj_t path_object) {
  char resolved_path[AREA512_PATH_MAX];

  resolve_path_or_raise(path_object, resolved_path, sizeof resolved_path);

  struct stat stat_buffer;

  if (stat(resolved_path, &stat_buffer) != 0)
    mp_raise_OSError(errno);

  size_t file_byte_size = (size_t)stat_buffer.st_size;

  if (file_byte_size == 0)
    return mp_obj_new_str("", 0);

  char *buffer = m_new(char, file_byte_size);
  FILE *file_pointer = fopen(resolved_path, "rb");

  if (file_pointer == NULL)
    mp_raise_OSError(errno);

  size_t read_byte_count = fread(buffer, 1, file_byte_size, file_pointer);

  fclose(file_pointer);

  if (read_byte_count != file_byte_size)
    mp_raise_type(&mp_type_OSError);

  mp_obj_t result_object = mp_obj_new_str(buffer, file_byte_size);

  m_del(char, buffer, file_byte_size);

  return result_object;
}
static MP_DEFINE_CONST_FUN_OBJ_1(
  read_whole_sd_file_callable,
  read_whole_sd_file
);

static mp_obj_t
write_whole_sd_file(mp_obj_t path_object, mp_obj_t content_object) {
  char resolved_path[AREA512_PATH_MAX];

  resolve_path_or_raise(path_object, resolved_path, sizeof resolved_path);

  size_t content_byte_length;
  const char *content =
    mp_obj_str_get_data(content_object, &content_byte_length);

  char temporary_path[AREA512_PATH_MAX + sizeof(".tmp")];

  strcpy(temporary_path, resolved_path);
  strcat(temporary_path, ".tmp");

  FILE *file_pointer = fopen(temporary_path, "wb");

  if (file_pointer == NULL)
    mp_raise_OSError(errno);

  size_t written_byte_count =
    fwrite(content, 1, content_byte_length, file_pointer);

  bool is_closed = fclose(file_pointer) == 0;

  if (!is_closed || written_byte_count != content_byte_length) {
    remove(temporary_path);
    mp_raise_type(&mp_type_OSError);
  }

  // FAT's rename can't replace an existing file, so remove the target first;
  // the fully written .tmp file survives a power cut in between.
  if (rename(temporary_path, resolved_path) != 0) {
    if (remove(resolved_path) != 0 ||
        rename(temporary_path, resolved_path) != 0) {

      remove(temporary_path);
      mp_raise_OSError(errno);
    }
  }

  return MP_OBJ_NEW_SMALL_INT(written_byte_count);
}
static MP_DEFINE_CONST_FUN_OBJ_2(
  write_whole_sd_file_callable,
  write_whole_sd_file
);

static mp_obj_t
restore_sd_seed(void) {
  if (area512_seed_restore() != 0)
    mp_raise_type(&mp_type_OSError);

  area512_theme_load();

  return mp_const_true;
}
static MP_DEFINE_CONST_FUN_OBJ_0(restore_sd_seed_callable, restore_sd_seed);

static const mp_rom_map_elem_t sd_locals_table[] = {
  {MP_ROM_QSTR(MP_QSTR_mount), MP_ROM_PTR(&mount_sd_callable)},
  {MP_ROM_QSTR(MP_QSTR_unmount), MP_ROM_PTR(&unmount_sd_callable)},
  {MP_ROM_QSTR(MP_QSTR_exist), MP_ROM_PTR(&is_existing_path_callable)},
  {
    MP_ROM_QSTR(MP_QSTR_mkdir),
    MP_ROM_PTR(&make_sd_directory_if_absent_callable),
  },
  {MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&read_whole_sd_file_callable)},
  {MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&write_whole_sd_file_callable)},
  {MP_ROM_QSTR(MP_QSTR_restore_seed), MP_ROM_PTR(&restore_sd_seed_callable)},
};
static MP_DEFINE_CONST_DICT(sd_locals_dictionary, sd_locals_table);

MP_DEFINE_CONST_OBJ_TYPE(
  area512_micropython_sd_type,
  MP_QSTR_SD,
  MP_TYPE_FLAG_NONE,
  locals_dict,
  &sd_locals_dictionary
);

static mp_obj_t
create_file_object(
  const mp_obj_type_t *file_type,
  size_t argument_count,
  size_t keyword_argument_count,
  const mp_obj_t *arguments
) {

  mp_arg_check_num(argument_count, keyword_argument_count, 1, 2, false);

  const char *mode = argument_count >= 2 && arguments[1] != mp_const_none
                       ? mp_obj_str_get_str(arguments[1])
                       : "r";
  char resolved_path[AREA512_PATH_MAX];

  resolve_path_or_raise(arguments[0], resolved_path, sizeof resolved_path);

  area512_micropython_file_instance_t *file_instance =
    mp_obj_malloc_with_finaliser(area512_micropython_file_instance_t, file_type);

  file_instance->file_pointer = NULL;

  FILE *file_pointer =
    fopen(resolved_path, strcmp(mode, "w") == 0 ? "wb" : "rb");

  if (file_pointer == NULL)
    mp_raise_OSError(errno);

  file_instance->file_pointer = file_pointer;

  return MP_OBJ_FROM_PTR(file_instance);
}

static mp_obj_t
unlink_file(mp_obj_t path_object) {
  char resolved_path[AREA512_PATH_MAX];

  resolve_path_or_raise(path_object, resolved_path, sizeof resolved_path);

  if (unlink(resolved_path) != 0)
    mp_raise_OSError(errno);

  return MP_OBJ_NEW_SMALL_INT(1);
}
static MP_DEFINE_CONST_FUN_OBJ_1(unlink_file_callable, unlink_file);

static mp_obj_t
rename_file(mp_obj_t source_path_object, mp_obj_t destination_path_object) {
  char resolved_source_path[AREA512_PATH_MAX];
  char resolved_destination_path[AREA512_PATH_MAX];

  resolve_path_or_raise(
    source_path_object,
    resolved_source_path,
    sizeof resolved_source_path
  );

  resolve_path_or_raise(
    destination_path_object,
    resolved_destination_path,
    sizeof resolved_destination_path
  );

  if (rename(resolved_source_path, resolved_destination_path) != 0)
    mp_raise_OSError(errno);

  return MP_OBJ_NEW_SMALL_INT(0);
}
static MP_DEFINE_CONST_FUN_OBJ_2(rename_file_callable, rename_file);

static mp_obj_t
read_remaining_file(mp_obj_t file_object) {
  FILE *file_pointer = fetch_file_pointer_or_raise(file_object);
  long start_position = ftell(file_pointer);

  if (start_position < 0 || fseek(file_pointer, 0, SEEK_END) != 0)
    mp_raise_OSError(errno);

  long end_position = ftell(file_pointer);

  if (end_position < 0 || fseek(file_pointer, start_position, SEEK_SET) != 0)
    mp_raise_OSError(errno);

  size_t remaining_byte_count = (size_t)(end_position - start_position);

  if (remaining_byte_count == 0)
    return mp_obj_new_str("", 0);

  char *buffer = m_new(char, remaining_byte_count);
  size_t read_byte_count = fread(buffer, 1, remaining_byte_count, file_pointer);

  if (read_byte_count != remaining_byte_count)
    mp_raise_type(&mp_type_OSError);

  mp_obj_t result_object = mp_obj_new_str(buffer, remaining_byte_count);

  m_del(char, buffer, remaining_byte_count);

  return result_object;
}
static MP_DEFINE_CONST_FUN_OBJ_1(
  read_remaining_file_callable,
  read_remaining_file
);

static mp_obj_t
close_file(mp_obj_t file_object) {
  area512_micropython_file_instance_t *file_instance =
    MP_OBJ_TO_PTR(file_object);

  if (file_instance->file_pointer != NULL) {
    fclose(file_instance->file_pointer);

    file_instance->file_pointer = NULL;
  }

  return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(close_file_callable, close_file);

static const mp_rom_map_elem_t file_locals_table[] = {
  {MP_ROM_QSTR(MP_QSTR_exist), MP_ROM_PTR(&is_existing_path_callable)},
  {MP_ROM_QSTR(MP_QSTR_file), MP_ROM_PTR(&is_regular_file_path_callable)},
  {MP_ROM_QSTR(MP_QSTR_directory), MP_ROM_PTR(&is_directory_path_callable)},
  {MP_ROM_QSTR(MP_QSTR_unlink), MP_ROM_PTR(&unlink_file_callable)},
  {MP_ROM_QSTR(MP_QSTR_rename), MP_ROM_PTR(&rename_file_callable)},
  {MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&read_remaining_file_callable)},
  {MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&close_file_callable)},
  {MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&close_file_callable)},
};
static MP_DEFINE_CONST_DICT(file_locals_dictionary, file_locals_table);

MP_DEFINE_CONST_OBJ_TYPE(
  area512_micropython_file_type,
  MP_QSTR_File,
  MP_TYPE_FLAG_NONE,
  make_new,
  create_file_object,
  locals_dict,
  &file_locals_dictionary
);

static mp_obj_t
create_directory_object(
  const mp_obj_type_t *directory_type,
  size_t argument_count,
  size_t keyword_argument_count,
  const mp_obj_t *arguments
) {

  mp_arg_check_num(argument_count, keyword_argument_count, 1, 1, false);

  char resolved_path[AREA512_PATH_MAX];

  resolve_path_or_raise(arguments[0], resolved_path, sizeof resolved_path);

  area512_micropython_directory_instance_t *directory_instance =
    mp_obj_malloc_with_finaliser(
      area512_micropython_directory_instance_t,
      directory_type
    );

  directory_instance->directory_pointer = NULL;

  DIR *directory_pointer = opendir(resolved_path);

  if (directory_pointer == NULL)
    mp_raise_OSError(errno);

  directory_instance->directory_pointer = directory_pointer;

  return MP_OBJ_FROM_PTR(directory_instance);
}

static mp_obj_t
make_directory(mp_obj_t path_object) {
  char resolved_path[AREA512_PATH_MAX];

  resolve_path_or_raise(path_object, resolved_path, sizeof resolved_path);

  if (mkdir(resolved_path, 0777) != 0)
    mp_raise_OSError(errno);

  return MP_OBJ_NEW_SMALL_INT(0);
}
static MP_DEFINE_CONST_FUN_OBJ_1(make_directory_callable, make_directory);

static mp_obj_t
remove_directory(mp_obj_t path_object) {
  char resolved_path[AREA512_PATH_MAX];

  resolve_path_or_raise(path_object, resolved_path, sizeof resolved_path);

  if (rmdir(resolved_path) != 0)
    mp_raise_OSError(errno);

  return MP_OBJ_NEW_SMALL_INT(0);
}
static MP_DEFINE_CONST_FUN_OBJ_1(remove_directory_callable, remove_directory);

static mp_obj_t
read_directory_entry(mp_obj_t directory_object) {
  struct dirent *entry =
    readdir(fetch_directory_pointer_or_raise(directory_object));

  if (entry == NULL)
    return mp_const_none;

  return mp_obj_new_str_from_cstr(entry->d_name);
}
static MP_DEFINE_CONST_FUN_OBJ_1(
  read_directory_entry_callable,
  read_directory_entry
);

static mp_obj_t
close_directory(mp_obj_t directory_object) {
  area512_micropython_directory_instance_t *directory_instance =
    MP_OBJ_TO_PTR(directory_object);

  if (directory_instance->directory_pointer != NULL) {
    closedir(directory_instance->directory_pointer);

    directory_instance->directory_pointer = NULL;
  }

  return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(close_directory_callable, close_directory);

static const mp_rom_map_elem_t directory_locals_table[] = {
  {MP_ROM_QSTR(MP_QSTR_exist), MP_ROM_PTR(&is_directory_path_callable)},
  {MP_ROM_QSTR(MP_QSTR_mkdir), MP_ROM_PTR(&make_directory_callable)},
  {MP_ROM_QSTR(MP_QSTR_rmdir), MP_ROM_PTR(&remove_directory_callable)},
  {MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&read_directory_entry_callable)},
  {MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&close_directory_callable)},
  {MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&close_directory_callable)},
};
static MP_DEFINE_CONST_DICT(
  directory_locals_dictionary,
  directory_locals_table
);

MP_DEFINE_CONST_OBJ_TYPE(
  area512_micropython_directory_type,
  MP_QSTR_Dir,
  MP_TYPE_FLAG_NONE,
  make_new,
  create_directory_object,
  locals_dict,
  &directory_locals_dictionary
);
