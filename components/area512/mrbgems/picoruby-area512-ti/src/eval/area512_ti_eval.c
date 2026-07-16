#include "area512_ti_eval.h"
#include "area512_ti_arena.h"
#include "area512_ti_bind.h"
#include "area512_ti_class.h"
#include "area512_ti_context.h"
#include "area512_ti_def.h"
#include "area512_ti_define_info.h"
#include "area512_ti_suggest.h"
#include "area512_ti_builtin.h"
#include "area512_ti_t.h"
#include "area512_ti_t_frame.h"
#include <prism.h>
#include <string.h>

typedef struct {
  const uint8_t *source;
  int cursor_byte_offset;
  pm_constant_id_t name;
  pm_location_t location;
} TiTypeSearch;

static bool
find_type_target(const pm_node_t *node, void *data) {
  TiTypeSearch *search = data;
  pm_constant_id_t name = 0;
  pm_location_t location = node->location;

  switch (PM_NODE_TYPE(node)) {
  case PM_LOCAL_VARIABLE_READ_NODE:
    name = ((const pm_local_variable_read_node_t *)node)->name;
    break;
  case PM_INSTANCE_VARIABLE_READ_NODE:
    name = ((const pm_instance_variable_read_node_t *)node)->name;
    break;
  case PM_GLOBAL_VARIABLE_READ_NODE:
    name = ((const pm_global_variable_read_node_t *)node)->name;
    break;
  case PM_CONSTANT_READ_NODE:
    name = ((const pm_constant_read_node_t *)node)->name;
    break;
  case PM_LOCAL_VARIABLE_WRITE_NODE: {
    const pm_local_variable_write_node_t *write =
      (const pm_local_variable_write_node_t *)node;
    name = write->name;
    location = write->name_loc;
    break;
  }
  case PM_INSTANCE_VARIABLE_WRITE_NODE: {
    const pm_instance_variable_write_node_t *write =
      (const pm_instance_variable_write_node_t *)node;
    name = write->name;
    location = write->name_loc;
    break;
  }
  case PM_GLOBAL_VARIABLE_WRITE_NODE: {
    const pm_global_variable_write_node_t *write =
      (const pm_global_variable_write_node_t *)node;
    name = write->name;
    location = write->name_loc;
    break;
  }
  case PM_CONSTANT_WRITE_NODE: {
    const pm_constant_write_node_t *write =
      (const pm_constant_write_node_t *)node;
    name = write->name;
    location = write->name_loc;
    break;
  }
  default:
    return true;
  }

  int start = (int)(location.start - search->source);
  int end = (int)(location.end - search->source);

  if (search->cursor_byte_offset < start || search->cursor_byte_offset >= end)
    return true;

  search->name = name;
  search->location = location;
  return false;
}

static bool
eval(const pm_node_t *node, void *data) {
  TiContext *context = data;

  if (context->failed)
    return false;

  switch (PM_NODE_TYPE(node)) {
  case PM_LOCAL_VARIABLE_WRITE_NODE: {
    const pm_local_variable_write_node_t *write =
      (const pm_local_variable_write_node_t *)node;

    ti_bind_scalar_assignment(context, write->name, write->value, 0);

    break;
  }

  case PM_INSTANCE_VARIABLE_WRITE_NODE: {
    const pm_instance_variable_write_node_t *write =
      (const pm_instance_variable_write_node_t *)node;

    ti_bind_scalar_assignment(context, write->name, write->value, 0);

    break;
  }

  case PM_GLOBAL_VARIABLE_WRITE_NODE: {
    const pm_global_variable_write_node_t *write =
      (const pm_global_variable_write_node_t *)node;

    ti_bind_scalar_assignment(context, write->name, write->value, 0);
    break;
  }

  case PM_CONSTANT_WRITE_NODE: {
    const pm_constant_write_node_t *write =
      (const pm_constant_write_node_t *)node;

    ti_bind_scalar_assignment(context, write->name, write->value, 0);

    break;
  }

  case PM_DEF_NODE:
    ti_eval_def(context, (const pm_def_node_t *)node);
    break;

  case PM_CLASS_NODE: {
    const pm_class_node_t *class_node = (const pm_class_node_t *)node;
    uint16_t class_name_id;

    if (!ti_convert_constant_id(class_node->name, &class_name_id)) {
      context->failed = 1;
      return false;
    }

    ti_eval_class(context, class_node);

    uint16_t outer_class_name_id = context->current_class_name_id;
    context->current_class_name_id = class_name_id;

    if (!context->failed && class_node->body)
      pm_visit_node(class_node->body, eval, context);

    context->current_class_name_id = outer_class_name_id;
    return false;
  }

  default:
    break;
  }

  return !context->failed;
}

static int
initialize_evaluator(void) {
  ti_reset_arena();

  return ti_initialize_t() && ti_initialize_t_frame() &&
         ti_initialize_define_infos();
}

int
ti_fill_suggestions_at_cursor(
  const char *source,
  int source_byte_length,
  int cursor_byte_offset,
  TiSuggestionList *out
) {
  if (out)
    memset(out, 0, sizeof(*out));

  if (!source || source_byte_length < 0 || !out || cursor_byte_offset < 0 ||
      cursor_byte_offset > source_byte_length) {
    return 0;
  }

  if (!initialize_evaluator())
    return 0;

  pm_parser_t parser;
  pm_parser_init(
    &parser,
    (const uint8_t *)source,
    (size_t)source_byte_length,
    NULL
  );
  pm_node_t *root = pm_parse(&parser);

  if (!root) {
    pm_parser_free(&parser);
    return 0;
  }

  TiContext context = {
    .parser = &parser,
    .source = (const uint8_t *)source,
    .source_length = (size_t)source_byte_length,
    .round = 1,
    .failed = 0,
  };

  pm_visit_node(root, eval, &context);
  context.round = 2;

  if (!context.failed)
    pm_visit_node(root, eval, &context);

  if (!context.failed && !ti_did_arena_overflow()) {
    ti_collect_suggestions(&context, root, cursor_byte_offset, out);
  }

  pm_node_destroy(&parser, root);
  pm_parser_free(&parser);

  if (context.failed || ti_did_arena_overflow()) {
    memset(out, 0, sizeof(*out));
    return 0;
  }

  return out->count;
}

static void
append_text(char *buffer, size_t capacity, size_t *length, const char *text) {
  if (!text)
    return;

  while (*text && *length + 1 < capacity)
    buffer[(*length)++] = *text++;

  if (capacity > 0)
    buffer[*length] = '\0';
}

static void
append_class_name(
  const TiContext *context,
  uint8_t class_id,
  char *buffer,
  size_t capacity,
  size_t *length
) {
  const char *builtin_name = ti_get_builtin_class_name(class_id);
  if (builtin_name) {
    append_text(buffer, capacity, length, builtin_name);
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

    for (size_t offset = 0;
         offset < constant->length && *length + 1 < capacity;
         offset++) {
      buffer[(*length)++] = (char)constant->start[offset];
    }
    buffer[*length] = '\0';
    return;
  }

  append_text(buffer, capacity, length, "untyped");
}

static void
format_single_t(
  const TiContext *context,
  const T *t,
  char *buffer,
  size_t capacity,
  size_t *length
) {
  append_class_name(
    context,
    t->object_class_id,
    buffer,
    capacity,
    length
  );

  if (t->object_class_id == TI_CLASS_ARRAY && t->variant1 != 0) {
    append_text(buffer, capacity, length, "<");
    const T *variant = ti_get_t(t->variant1);
    if (variant)
      format_single_t(context, variant, buffer, capacity, length);
    append_text(buffer, capacity, length, ">");
  }
}

static int
format_t(
  const TiContext *context,
  uint16_t t_index,
  char *buffer,
  size_t capacity
) {
  if (!buffer || capacity == 0)
    return 0;

  buffer[0] = '\0';
  const T *first = ti_get_t(t_index);
  if (!first)
    return 0;

  size_t length = 0;
  int is_union = first->union_next != 0;
  if (is_union)
    append_text(buffer, capacity, &length, "Union<");

  const T *current = first;
  while (current) {
    if (current != first)
      append_text(buffer, capacity, &length, " ");

    format_single_t(context, current, buffer, capacity, &length);
    current = ti_get_t(current->union_next);
  }

  if (is_union)
    append_text(buffer, capacity, &length, ">");

  return (int)length;
}

int
ti_find_type_at_cursor(
  const char *source,
  int source_byte_length,
  int cursor_byte_offset,
  TiTypeInfo *out
) {
  if (out)
    memset(out, 0, sizeof(*out));

  if (!source || source_byte_length < 0 || !out || cursor_byte_offset < 0 ||
      cursor_byte_offset > source_byte_length || !initialize_evaluator()) {
    return 0;
  }

  pm_parser_t parser;
  pm_parser_init(
    &parser,
    (const uint8_t *)source,
    (size_t)source_byte_length,
    NULL
  );
  pm_node_t *root = pm_parse(&parser);
  if (!root) {
    pm_parser_free(&parser);
    return 0;
  }

  TiContext context = {
    .parser = &parser,
    .source = (const uint8_t *)source,
    .source_length = (size_t)source_byte_length,
    .round = 1,
  };

  pm_visit_node(root, eval, &context);
  context.round = 2;
  if (!context.failed)
    pm_visit_node(root, eval, &context);

  TiTypeSearch search = {
    .source = (const uint8_t *)source,
    .cursor_byte_offset = cursor_byte_offset,
  };
  if (!context.failed && !ti_did_arena_overflow())
    pm_visit_node(root, find_type_target, &search);

  uint16_t name_id;
  if (search.name != 0 && ti_convert_constant_id(search.name, &name_id)) {
    const pm_constant_t *constant = ti_get_constant(&context, search.name);
    uint16_t t_index = ti_get_value_t(name_id);

    if (constant && t_index != 0 &&
        format_t(
          &context,
          t_index,
          out->type_name,
          sizeof(out->type_name)
        ) > 0) {
      size_t name_length = constant->length;
      if (name_length >= sizeof(out->variable_name))
        name_length = sizeof(out->variable_name) - 1;
      memcpy(out->variable_name, constant->start, name_length);
      out->variable_name[name_length] = '\0';
      out->found = 1;
    }
  }

  pm_node_destroy(&parser, root);
  pm_parser_free(&parser);
  return out->found;
}
