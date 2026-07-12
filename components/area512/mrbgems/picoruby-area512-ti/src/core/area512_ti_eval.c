#include "area512_ti_eval.h"
#include "area512_ti_arena.h"
#include "area512_ti_builtin_db.h"
#include "area512_ti_context.h"
#include "area512_ti_define_info.h"
#include "area512_ti_eval_expression.h"
#include "area512_ti_suggest.h"
#include "area512_ti_t.h"
#include "area512_ti_t_frame.h"
#include <prism.h>
#include <stdint.h>
#include <string.h>

int
ti_convert_constant_id(pm_constant_id_t constant_id, uint16_t *name_id) {
  if (!name_id || constant_id == 0 || constant_id > UINT16_MAX)
    return 0;

  *name_id = (uint16_t)constant_id;
  return 1;
}

const pm_constant_t *
ti_get_constant(const TiContext *context, pm_constant_id_t constant_id) {
  if (!context || !context->parser || constant_id == 0)
    return NULL;

  return pm_constant_pool_id_to_constant(
    &context->parser->constant_pool,
    constant_id
  );
}

static uint16_t
calculate_row(const TiContext *context, const uint8_t *location) {

  uint16_t define_row = 1;

  for (const uint8_t *cursor = context->source;
       cursor < location && cursor < context->source + context->source_length;
       cursor++) {
    if (*cursor == '\n' && define_row < UINT16_MAX)
      define_row++;
  }

  return define_row;
}

static void
handle_scalar_assignment(
  TiContext *context,
  pm_constant_id_t constant_id,
  const pm_node_t *value
) {

  uint16_t name_id;

  if (!ti_convert_constant_id(constant_id, &name_id)) {
    context->failed = 1;
    return;
  }

  uint16_t t_index = ti_eval_expression(context, value, 0);

  if (t_index != 0 && !ti_set_value_t(name_id, t_index))
    context->failed = 1;
}

static void
set_define_args(
  TiContext *context,
  TiDefineInfo *define_info,
  const pm_parameters_node_t *parameters
) {
  if (!define_info || !parameters)
    return;

  size_t required_count = parameters->requireds.size;
  if (required_count > TI_DEFINE_ARG_CAPACITY)
    required_count = TI_DEFINE_ARG_CAPACITY;

  define_info->define_arg_count = (uint8_t)required_count;

  for (size_t index = 0; index < required_count; index++) {
    const pm_node_t *parameter = parameters->requireds.nodes[index];

    if (PM_NODE_TYPE(parameter) != PM_REQUIRED_PARAMETER_NODE)
      continue;

    pm_constant_id_t constant_id =
      ((const pm_required_parameter_node_t *)parameter)->name;
    uint16_t name_id;

    if (!ti_convert_constant_id(constant_id, &name_id)) {
      context->failed = 1;
      return;
    }

    define_info->define_arg_name_ids[index] = name_id;
  }
}

static void
eval_def(TiContext *context, const pm_def_node_t *def_node) {
  uint16_t name_id;

  if (!ti_convert_constant_id(def_node->name, &name_id)) {
    context->failed = 1;
    return;
  }

  uint16_t define_row = calculate_row(context, def_node->base.location.start);
  TiDefineInfo *define_info = ti_set_define_info(name_id, define_row, 0);

  if (!define_info)
    return;

  set_define_args(context, define_info, def_node->parameters);

  uint16_t return_t_index = ti_eval_expression(context, def_node->body, 0);

  if (return_t_index == 0)
    return;

  define_info->return_t_index = return_t_index;
  if (!ti_set_value_t(name_id, return_t_index))
    context->failed = 1;
}

static void
eval_class(TiContext *context, const pm_class_node_t *class_node) {
  uint16_t name_id;

  if (!ti_convert_constant_id(class_node->name, &name_id)) {
    context->failed = 1;
    return;
  }

  uint16_t define_row = calculate_row(context, class_node->base.location.start);

  TiDefineInfo *define_info = ti_set_define_info(name_id, define_row, 1);

  if (!define_info)
    return;

  uint16_t class_t_index = ti_new_t(TI_CLASS_CLASS, TI_T_FLAG_STATIC, 0, 0);

  if (class_t_index == 0 || !ti_set_value_t(name_id, class_t_index))
    context->failed = 1;
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

    handle_scalar_assignment(context, write->name, write->value);

    break;
  }

  case PM_INSTANCE_VARIABLE_WRITE_NODE: {
    const pm_instance_variable_write_node_t *write =
      (const pm_instance_variable_write_node_t *)node;

    handle_scalar_assignment(context, write->name, write->value);

    break;
  }

  case PM_GLOBAL_VARIABLE_WRITE_NODE: {
    const pm_global_variable_write_node_t *write =
      (const pm_global_variable_write_node_t *)node;

    handle_scalar_assignment(context, write->name, write->value);
    break;
  }

  case PM_CONSTANT_WRITE_NODE: {
    const pm_constant_write_node_t *write =
      (const pm_constant_write_node_t *)node;

    handle_scalar_assignment(context, write->name, write->value);

    break;
  }

  case PM_DEF_NODE:
    eval_def(context, (const pm_def_node_t *)node);
    break;

  case PM_CLASS_NODE:
    eval_class(context, (const pm_class_node_t *)node);
    break;

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
