#include "area512_ti_type.h"
#include "area512_ti_builtin.h"
#include "area512_ti_define_info.h"
#include "area512_ti_t.h"
#include <string.h>

static void
write_text(
  char *buffer,
  size_t capacity,
  size_t *length,
  const char *text,
  size_t text_length
) {
  if (!text)
    return;

  for (size_t offset = 0; offset < text_length && *length + 1 < capacity;
       offset++) {
    buffer[(*length)++] = text[offset];
  }

  if (capacity > 0)
    buffer[*length] = '\0';
}

static void
object_class_to_string(
  const TiContext *context,
  uint8_t class_id,
  char *buffer,
  size_t capacity,
  size_t *length
) {

  const char *builtin_name = ti_get_builtin_class_name(class_id);
  if (builtin_name) {
    write_text(
      buffer,
      capacity,
      length,
      builtin_name,
      strlen(builtin_name)
    );

    return;
  }

  int user_class_index = (int)class_id - TI_CLASS_USER_BASE;
  int current_user_class_index = 0;

  for (int index = 0; index < ti_get_define_info_count(); index++) {
    TiDefineInfo *define_info = ti_get_define_info(index);

    if (!define_info || !define_info->is_class)
      continue;

    if (current_user_class_index++ != user_class_index)
      continue;

    const pm_constant_t *constant =
      ti_get_constant(context, define_info->name_id);

    if (!constant)
      break;

    write_text(
      buffer,
      capacity,
      length,
      (const char *)constant->start,
      constant->length
    );

    return;
  }

  write_text(
    buffer,
    capacity,
    length,
    "untyped",
    sizeof("untyped") - 1
  );
}

static void type_to_string(
  const TiContext *context,
  const T *t,
  char *buffer,
  size_t capacity,
  size_t *length
);

static void
array_type_to_string(
  const TiContext *context,
  const T *t,
  char *buffer,
  size_t capacity,
  size_t *length
) {

  object_class_to_string(context, t->object_class_id, buffer, capacity, length);
  write_text(buffer, capacity, length, "<", sizeof("<") - 1);

  const T *variant = ti_get_t(t->variants);

  if (variant)
    type_to_string(context, variant, buffer, capacity, length);

  write_text(buffer, capacity, length, ">", sizeof(">") - 1);
}

static void
type_to_string(
  const TiContext *context,
  const T *t,
  char *buffer,
  size_t capacity,
  size_t *length
) {

  if (t->object_class_id == TI_CLASS_ARRAY && t->variants != 0) {
    array_type_to_string(context, t, buffer, capacity, length);
    return;
  }

  object_class_to_string(context, t->object_class_id, buffer, capacity, length);
}

static void
union_type_to_string(
  const TiContext *context,
  const T *first_t_node,
  char *buffer,
  size_t capacity,
  size_t *length
) {

  write_text(
    buffer,
    capacity,
    length,
    "Union<",
    sizeof("Union<") - 1
  );

  for (const T *current = first_t_node; current;
       current = ti_get_t(current->union_next)) {

    if (current != first_t_node)
      write_text(
        buffer,
        capacity,
        length,
        " ",
        sizeof(" ") - 1
      );

    type_to_string(context, current, buffer, capacity, length);
  }

  write_text(buffer, capacity, length, ">", sizeof(">") - 1);
}

int
ti_type_to_string(
  const TiContext *context,
  uint16_t t_node_index,
  char *buffer,
  size_t capacity
) {

  if (!buffer || capacity == 0)
    return 0;

  buffer[0] = '\0';

  const T *first_t_node = ti_get_t(t_node_index);
  if (!first_t_node)
    return 0;

  size_t length = 0;

  if (first_t_node->union_next != 0)
    union_type_to_string(context, first_t_node, buffer, capacity, &length);
  else
    type_to_string(context, first_t_node, buffer, capacity, &length);

  return (int)length;
}
