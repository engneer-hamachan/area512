#include "area512_ti_hover.h"
#include "area512_ti_arena.h"
#include "area512_ti_context.h"
#include "area512_ti_eval.h"
#include "area512_ti_t_frame.h"
#include "area512_ti_type.h"
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
      cursor_byte_offset > source_byte_length) {
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
  };

  TiTypeSearch search = {
    .source = (const uint8_t *)source,
    .cursor_byte_offset = cursor_byte_offset,
  };

  if (ti_evaluation_loop(&context, root) && !ti_did_arena_overflow())
    pm_visit_node(root, find_type_target, &search);

  uint16_t name_id;
  if (search.name != 0 && ti_convert_constant_id(search.name, &name_id)) {
    const pm_constant_t *constant = ti_get_constant(&context, search.name);
    uint16_t t_index = ti_get_value_t(name_id);

    if (constant && t_index != 0 &&
        ti_type_to_string(
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
