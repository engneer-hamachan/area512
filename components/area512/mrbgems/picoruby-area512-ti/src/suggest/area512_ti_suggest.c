#include "area512_ti_suggest.h"
#include "area512_ti_arena.h"
#include "area512_ti_builtin.h"
#include "area512_ti_context.h"
#include "area512_ti_define_info.h"
#include "area512_ti_eval.h"
#include "area512_ti_t.h"
#include "area512_ti_type.h"
#include <stddef.h>
#include <string.h>

typedef struct {
  const uint8_t *expected_end;
  const pm_node_t *target;
  size_t target_length;
} SuggestTargetSearch;

typedef struct {
  const uint8_t *cursor;
  const pm_class_node_t *target;
  size_t target_length;
} EnclosingClassSearch;

static int
is_identifier_byte(uint8_t byte) {
  return (byte >= 'a' && byte <= 'z') || (byte >= 'A' && byte <= 'Z') ||
         (byte >= '0' && byte <= '9') || byte == '_' || byte == '!' ||
         byte == '?';
}

static int
find_suggest_prefix(
  const TiContext *context,
  int cursor_byte_offset,
  size_t *dot_offset,
  const uint8_t **prefix,
  size_t *prefix_length,
  int *has_receiver
) {

  size_t cursor = (size_t)cursor_byte_offset;
  size_t start = cursor;

  while (start > 0 && is_identifier_byte(context->source[start - 1]))
    start--;

  *has_receiver = start > 0 && context->source[start - 1] == '.';
  *dot_offset = *has_receiver ? start - 1 : start;
  *prefix = context->source + start;
  *prefix_length = cursor - start;

  return 1;
}

static bool
find_enclosing_class_on_visit(const pm_node_t *node, void *data) {
  EnclosingClassSearch *search = data;

  if (PM_NODE_TYPE(node) != PM_CLASS_NODE)
    return true;

  if (node->location.start > search->cursor ||
      node->location.end < search->cursor) {

    return true;
  }

  size_t length = (size_t)(node->location.end - node->location.start);
  if (!search->target || length < search->target_length) {
    search->target = (const pm_class_node_t *)node;
    search->target_length = length;
  }

  return true;
}

static bool
find_suggest_target_on_visit(const pm_node_t *node, void *data) {
  SuggestTargetSearch *search = data;

  if (node->location.end != search->expected_end)
    return true;

  size_t length = (size_t)(node->location.end - node->location.start);

  if (!search->target || length < search->target_length) {
    search->target = node;
    search->target_length = length;
  }

  return true;
}

static int
matches_prefix(
  const uint8_t *name,
  size_t name_length,
  const uint8_t *prefix,
  size_t prefix_length
) {
  return name_length >= prefix_length &&
         memcmp(name, prefix, prefix_length) == 0;
}

static int
has_suggestion(
  const TiSuggestionList *out,
  const char *contents,
  const char *detail
) {

  for (int index = 0; index < out->count; index++) {
    const TiSuggestion *suggestion = &out->items[index];

    if (strcmp(suggestion->contents, contents) == 0 &&
        strcmp(suggestion->detail, detail) == 0) {
      return 1;
    }
  }

  return 0;
}

static void
append_builtin_suggestions(
  uint8_t class_id,
  int use_static_methods,
  int show_class_name,
  const uint8_t *prefix,
  size_t prefix_length,
  TiSuggestionList *out
) {

  const TiBuiltinMethod *methods[TI_MAX_SUGGESTIONS];
  if (out->count >= TI_MAX_SUGGESTIONS)
    return;

  int method_count =
    ti_collect_builtin_methods_matching_partial_method_name(
      class_id,
      use_static_methods,
      prefix,
      prefix_length,
      methods,
      TI_MAX_SUGGESTIONS
    );

  for (int index = 0; index < method_count; index++) {
    const TiBuiltinMethod *method = methods[index];
    const char *name = ti_get_builtin_method_name(method);
    const char *signature = ti_get_builtin_signature(method);

    if (has_suggestion(out, name, signature))
      continue;

    if (out->count >= TI_MAX_SUGGESTIONS)
      return;

    TiSuggestion *suggestion = &out->items[out->count++];
    suggestion->detail = signature;
    suggestion->document = ti_get_builtin_document(method);
    suggestion->contents = name;
    suggestion->contents_length = (int)strlen(name);

    suggestion->class_name =
      show_class_name ? ti_get_builtin_class_name(class_id) : NULL;

    if (out->count >= TI_MAX_SUGGESTIONS)
      return;
  }
}

static char *
copy_constant_to_arena(const pm_constant_t *constant) {
  char *copy = ti_allocate_from_arena(constant->length + 1);
  if (!copy)
    return NULL;

  memcpy(copy, constant->start, constant->length);
  copy[constant->length] = '\0';

  return copy;
}

static char *
make_signature_content(
  TiContext *context,
  const TiDefineInfo *define_info,
  const pm_constant_t *name
) {

  char return_type[TI_TYPE_STRING_CAPACITY];

  size_t return_type_length =
    (size_t)ti_type_to_string(
      context,
      define_info->return_t_node_index,
      return_type,
      sizeof(return_type)
    );

  if (return_type_length == 0) {
    memcpy(return_type, "untyped", sizeof("untyped"));
    return_type_length = sizeof("untyped") - 1;
  }

  size_t length = name->length + 2;

  for (int index = 0; index < define_info->define_arg_count; index++) {
    const pm_constant_t *define_arg =
      ti_get_constant(context, define_info->define_arg_name_ids[index]);

    if (define_arg)
      length += define_arg->length;

    if (index > 0)
      length += 2;
  }

  length += sizeof(" -> ") - 1 + return_type_length;

  char *detail = ti_allocate_from_arena(length + 1);
  if (!detail)
    return NULL;

  size_t offset = 0;
  memcpy(detail + offset, name->start, name->length);
  offset += name->length;
  detail[offset++] = '(';

  for (int index = 0; index < define_info->define_arg_count; index++) {
    const pm_constant_t *define_arg =
      ti_get_constant(context, define_info->define_arg_name_ids[index]);

    if (index > 0) {
      detail[offset++] = ',';
      detail[offset++] = ' ';
    }

    if (define_arg) {
      memcpy(detail + offset, define_arg->start, define_arg->length);
      offset += define_arg->length;
    }
  }

  detail[offset++] = ')';
  memcpy(detail + offset, " -> ", sizeof(" -> ") - 1);
  offset += sizeof(" -> ") - 1;
  memcpy(detail + offset, return_type, return_type_length);
  offset += return_type_length;
  detail[offset] = '\0';

  return detail;
}

static void
append_define_info_suggestions_for_owner(
  TiContext *context,
  uint16_t class_name_id,
  const uint8_t *prefix,
  size_t prefix_length,
  TiSuggestionList *out
) {

  for (int index = 0;
       index < ti_get_define_info_count() && out->count < TI_MAX_SUGGESTIONS;
       index++) {

    TiDefineInfo *define_info = ti_get_define_info(index);

    if (
      !define_info ||
      define_info->is_class ||
      define_info->owner_class_name_id != class_name_id
    ) {

      continue;
    }

    const pm_constant_t *name = ti_get_constant(context, define_info->name_id);

    if (
      !name ||
      !matches_prefix(name->start, name->length, prefix, prefix_length)
    ) {
      continue;
    }

    char *contents = copy_constant_to_arena(name);
    char *detail = make_signature_content(context, define_info, name);

    if (!contents || !detail) {
      context->failed = 1;
      return;
    }

    if (has_suggestion(out, contents, detail))
      continue;

    TiSuggestion *suggestion = &out->items[out->count++];
    suggestion->detail = detail;
    suggestion->document = "";
    suggestion->contents = contents;
    suggestion->contents_length = (int)name->length;
    suggestion->class_name = NULL;
  }
}

static void
append_define_info_suggestions(
  TiContext *context,
  uint8_t class_id,
  const uint8_t *prefix,
  size_t prefix_length,
  TiSuggestionList *out
) {

  int user_class_index = class_id - TI_CLASS_USER_BASE;
  int current_class_index = 0;

  for (int index = 0; index < ti_get_define_info_count(); index++) {
    TiDefineInfo *define_info = ti_get_define_info(index);

    if (!define_info || !define_info->is_class)
      continue;

    if (current_class_index == user_class_index) {
      append_define_info_suggestions_for_owner(
        context,
        define_info->name_id,
        prefix,
        prefix_length,
        out
      );

      return;
    }

    current_class_index++;
  }
}

static int
collect_suggestions(
  TiContext *context,
  const pm_node_t *root,
  int cursor_byte_offset,
  TiSuggestionList *out
) {

  size_t dot_offset;
  const uint8_t *prefix;
  size_t prefix_length;
  int has_receiver;

  if (!find_suggest_prefix(
        context,
        cursor_byte_offset,
        &dot_offset,
        &prefix,
        &prefix_length,
        &has_receiver
      )) {

    return 0;
  }

  if (!has_receiver) {
    append_define_info_suggestions_for_owner(
      context,
      0,
      prefix,
      prefix_length,
      out
    );

    EnclosingClassSearch class_search = {
      .cursor = context->source + cursor_byte_offset,
      .target = NULL,
      .target_length = 0,
    };

    pm_visit_node(root, find_enclosing_class_on_visit, &class_search);

    if (class_search.target) {
      uint16_t class_name_id;

      if (ti_convert_constant_id(class_search.target->name, &class_name_id)) {

        uint8_t class_id = ti_get_defined_class_id(class_name_id);

        if (class_id != TI_CLASS_NONE) {
          append_define_info_suggestions(
            context,
            class_id,
            prefix,
            prefix_length,
            out
          );
        }
      }
    }

    append_builtin_suggestions(
      TI_CLASS_OBJECT,
      0,
      0,
      prefix,
      prefix_length,
      out
    );

    append_builtin_suggestions(
      TI_CLASS_KERNEL,
      0,
      0,
      prefix,
      prefix_length,
      out
    );

    return out->count;
  }

  SuggestTargetSearch search = {
    .expected_end = context->source + dot_offset,
    .target = NULL,
    .target_length = 0,
  };

  pm_visit_node(root, find_suggest_target_on_visit, &search);

  if (!search.target)
    return 0;

  uint16_t target_t_node_index = ti_eval_expression(context, search.target, 0);
  const T *target_t = ti_get_t(target_t_node_index);

  if (!target_t)
    return 0;

  int show_class_name = target_t->union_next != 0;

  while (target_t) {
    if ((target_t->t_flags & TI_T_FLAG_DEFINED_CLASS) != 0) {
      if (!show_class_name && (target_t->t_flags & TI_T_FLAG_STATIC) == 0) {
        append_define_info_suggestions(
          context,
          target_t->object_class_id,
          prefix,
          prefix_length,
          out
        );
      }
    } else if (target_t->object_class_id != TI_CLASS_UNTYPED) {
      append_builtin_suggestions(
        target_t->object_class_id,
        (target_t->t_flags & TI_T_FLAG_STATIC) != 0,
        show_class_name,
        prefix,
        prefix_length,
        out
      );
    }

    target_t = ti_get_t(target_t->union_next);
  }

  return out->count;
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

  if (ti_evaluation_loop(&context, root) && !ti_did_arena_overflow()) {
    collect_suggestions(&context, root, cursor_byte_offset, out);
  }

  pm_node_destroy(&parser, root);
  pm_parser_free(&parser);

  if (context.failed || ti_did_arena_overflow()) {
    memset(out, 0, sizeof(*out));
    return 0;
  }

  return out->count;
}
