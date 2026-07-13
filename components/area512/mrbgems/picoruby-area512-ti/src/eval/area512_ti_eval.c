#include "area512_ti_eval.h"
#include "area512_ti_arena.h"
#include "area512_ti_bind.h"
#include "area512_ti_class.h"
#include "area512_ti_context.h"
#include "area512_ti_def.h"
#include "area512_ti_define_info.h"
#include "area512_ti_suggest.h"
#include "area512_ti_t.h"
#include "area512_ti_t_frame.h"
#include <prism.h>
#include <string.h>

static bool
eval(const pm_node_t *node, void *data) {
  TiContext *context = data;

  if (context->failed)
    return false;

  switch (PM_NODE_TYPE(node)) {
  case PM_LOCAL_VARIABLE_WRITE_NODE: {
    const pm_local_variable_write_node_t *write =
      (const pm_local_variable_write_node_t *)node;

    ti_bind_scalar_assignment(context, write->name, write->value);

    break;
  }

  case PM_INSTANCE_VARIABLE_WRITE_NODE: {
    const pm_instance_variable_write_node_t *write =
      (const pm_instance_variable_write_node_t *)node;

    ti_bind_scalar_assignment(context, write->name, write->value);

    break;
  }

  case PM_GLOBAL_VARIABLE_WRITE_NODE: {
    const pm_global_variable_write_node_t *write =
      (const pm_global_variable_write_node_t *)node;

    ti_bind_scalar_assignment(context, write->name, write->value);
    break;
  }

  case PM_CONSTANT_WRITE_NODE: {
    const pm_constant_write_node_t *write =
      (const pm_constant_write_node_t *)node;

    ti_bind_scalar_assignment(context, write->name, write->value);

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
