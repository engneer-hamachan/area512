#include "core/syntax/python/highlight.h"
#include "area512_hal.h"
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <tree_sitter/api.h>
#include <tree_sitter/tree-sitter-python.h>

typedef enum {
  EDIT_PYTHON_HIGHLIGHT_DEFAULT = 0,
  EDIT_PYTHON_HIGHLIGHT_KEYWORD,
  EDIT_PYTHON_HIGHLIGHT_BUILTIN_LITERAL,
  EDIT_PYTHON_HIGHLIGHT_CLASS,
  EDIT_PYTHON_HIGHLIGHT_FUNCTION,
  EDIT_PYTHON_HIGHLIGHT_BUILTIN_FUNCTION,
  EDIT_PYTHON_HIGHLIGHT_STRING,
  EDIT_PYTHON_HIGHLIGHT_NUMBER,
  EDIT_PYTHON_HIGHLIGHT_COMMENT,
} editor_python_highlight_type_t;

typedef struct {
  const uint8_t *source;
  int source_byte_length;
  void (*write_segment)(
    void *writer_context,
    const char *text,
    int text_byte_length,
    uint32_t color
  );
  void *writer_context;
  int last_end_byte_offset;
  const char *previous_node_type;
} editor_python_highlight_context_t;

static const uint32_t EDITOR_PYTHON_HIGHLIGHT_COLORS[] = {
  0,
  0xC586C0,
  0x569CD6,
  0x4EC9B0,
  0xDCDCAA,
  0xDCDCAA,
  0xCE9178,
  0xB5CEA8,
  0x6A9955,
};

static const char *const PYTHON_BUILTIN_FUNCTION_NAMES[] = {
  "abs",          "all",         "any",        "ascii",      "bin",
  "bool",         "breakpoint",  "bytearray",  "bytes",      "callable",
  "chr",          "classmethod", "compile",    "complex",    "delattr",
  "dict",         "dir",         "divmod",     "enumerate",  "eval",
  "exec",         "filter",      "float",      "format",     "frozenset",
  "getattr",      "globals",     "hasattr",    "hash",       "help",
  "hex",          "id",          "input",      "int",        "isinstance",
  "issubclass",   "iter",        "len",        "list",       "locals",
  "map",          "max",         "memoryview", "min",        "next",
  "object",       "oct",         "open",       "ord",        "pow",
  "print",        "property",    "range",      "repr",       "reversed",
  "round",        "set",         "setattr",    "slice",      "sorted",
  "staticmethod", "str",         "sum",        "super",      "tuple",
  "type",         "vars",        "zip",        "__import__",
};

#define PYTHON_BUILTIN_FUNCTION_NAME_COUNT                                     \
  (sizeof(PYTHON_BUILTIN_FUNCTION_NAMES) /                                     \
   sizeof(PYTHON_BUILTIN_FUNCTION_NAMES[0]))

static const char *const PYTHON_KEYWORD_NODE_TYPES[] = {
  "as",       "assert",  "async", "await",  "break",  "class",
  "continue", "def",     "del",   "elif",   "else",   "except",
  "exec",     "finally", "for",   "from",   "global", "if",
  "import",   "in",      "is",    "lambda", "match",  "nonlocal",
  "not",      "or",      "and",   "pass",   "print",  "raise",
  "return",   "try",     "while", "with",   "yield",  "case",
};

#define PYTHON_KEYWORD_NODE_TYPE_COUNT                                         \
  (sizeof(PYTHON_KEYWORD_NODE_TYPES) / sizeof(PYTHON_KEYWORD_NODE_TYPES[0]))

static bool
python_node_text_equals(
  const editor_python_highlight_context_t *context,
  TSNode node,
  const char *expected_text
) {

  uint32_t start_byte_offset = ts_node_start_byte(node);
  uint32_t end_byte_offset = ts_node_end_byte(node);
  uint32_t expected_text_byte_length = (uint32_t)strlen(expected_text);

  return end_byte_offset >= start_byte_offset &&
         end_byte_offset <= (uint32_t)context->source_byte_length &&
         end_byte_offset - start_byte_offset == expected_text_byte_length &&
         memcmp(
           context->source + start_byte_offset,
           expected_text,
           expected_text_byte_length
         ) == 0;
}

static bool
python_node_starts_with_uppercase_letter(
  const editor_python_highlight_context_t *context,
  TSNode node
) {
  uint32_t start_byte_offset = ts_node_start_byte(node);

  if (start_byte_offset >= (uint32_t)context->source_byte_length)
    return false;

  uint8_t first_byte = context->source[start_byte_offset];

  return first_byte >= 'A' && first_byte <= 'Z';
}

static bool
python_node_is_builtin_function_name(
  const editor_python_highlight_context_t *context,
  TSNode node
) {

  for (
    size_t function_name_index = 0;
    function_name_index < PYTHON_BUILTIN_FUNCTION_NAME_COUNT;
    function_name_index++
  ) {

    if (
      python_node_text_equals(
        context,
        node,
        PYTHON_BUILTIN_FUNCTION_NAMES[function_name_index]
      )
    ) {

      return true;
    }
  }

  return false;
}

static editor_python_highlight_type_t
python_highlight_type_for_identifier(
  const editor_python_highlight_context_t *context,
  TSNode node
) {

  if (strcmp(context->previous_node_type, "def") == 0)
    return EDIT_PYTHON_HIGHLIGHT_FUNCTION;

  if (strcmp(context->previous_node_type, "class") == 0)
    return EDIT_PYTHON_HIGHLIGHT_CLASS;

  if (python_node_starts_with_uppercase_letter(context, node))
    return EDIT_PYTHON_HIGHLIGHT_CLASS;

  if (python_node_is_builtin_function_name(context, node))
    return EDIT_PYTHON_HIGHLIGHT_BUILTIN_FUNCTION;

  return EDIT_PYTHON_HIGHLIGHT_DEFAULT;
}

static editor_python_highlight_type_t
python_highlight_type_for_node(
  const editor_python_highlight_context_t *context,
  TSNode node
) {

  const char *node_type = ts_node_type(node);

  if (strcmp(node_type, "comment") == 0)
    return EDIT_PYTHON_HIGHLIGHT_COMMENT;

  if (strcmp(node_type, "string") == 0 ||
      strcmp(node_type, "concatenated_string") == 0) {
    return EDIT_PYTHON_HIGHLIGHT_STRING;
  }

  if (strcmp(node_type, "integer") == 0 || strcmp(node_type, "float") == 0)
    return EDIT_PYTHON_HIGHLIGHT_NUMBER;

  if (
    strcmp(node_type, "none") == 0 ||
    strcmp(node_type, "true") == 0 ||
    strcmp(node_type, "false") == 0
  ) {

    return EDIT_PYTHON_HIGHLIGHT_BUILTIN_LITERAL;
  }

  for (
    size_t node_type_index = 0;
    node_type_index < PYTHON_KEYWORD_NODE_TYPE_COUNT;
    node_type_index++
  ) {

    if (strcmp(node_type, PYTHON_KEYWORD_NODE_TYPES[node_type_index]) == 0)
      return EDIT_PYTHON_HIGHLIGHT_KEYWORD;
  }

  if (strcmp(node_type, "identifier") == 0)
    return python_highlight_type_for_identifier(context, node);

  return EDIT_PYTHON_HIGHLIGHT_DEFAULT;
}

static void
write_python_highlight_segment(
  editor_python_highlight_context_t *context,
  editor_python_highlight_type_t highlight_type,
  int start_byte_offset,
  int end_byte_offset
) {

  if (start_byte_offset > context->last_end_byte_offset) {
    context->write_segment(
      context->writer_context,
      (const char *)(context->source + context->last_end_byte_offset),
      start_byte_offset - context->last_end_byte_offset,
      area512_theme_text_color()
    );
  }

  if (end_byte_offset > start_byte_offset) {
    uint32_t segment_color = highlight_type == EDIT_PYTHON_HIGHLIGHT_DEFAULT
                               ? area512_theme_text_color()
                               : EDITOR_PYTHON_HIGHLIGHT_COLORS[highlight_type];

    context->write_segment(
      context->writer_context,
      (const char *)(context->source + start_byte_offset),
      end_byte_offset - start_byte_offset,
      segment_color
    );
  }

  context->last_end_byte_offset = end_byte_offset;
}

static void
highlight_python_node(
  editor_python_highlight_context_t *context,
  TSNode node,
  editor_python_highlight_type_t highlight_type
) {

  if (ts_node_is_missing(node))
    return;

  if (highlight_type != EDIT_PYTHON_HIGHLIGHT_DEFAULT) {
    int start_byte_offset = (int)ts_node_start_byte(node);
    int end_byte_offset = (int)ts_node_end_byte(node);

    if (start_byte_offset < context->last_end_byte_offset)
      return;

    if (end_byte_offset > context->source_byte_length)
      end_byte_offset = context->source_byte_length;

    write_python_highlight_segment(
      context,
      highlight_type,
      start_byte_offset,
      end_byte_offset
    );

    context->previous_node_type = ts_node_type(node);

    return;
  }

  uint32_t child_count = ts_node_child_count(node);

  if (child_count == 0) {
    context->previous_node_type = ts_node_type(node);

    return;
  }

  for (uint32_t child_index = 0; child_index < child_count; child_index++) {
    TSNode child_node = ts_node_child(node, child_index);

    editor_python_highlight_type_t child_highlight_type =
      python_highlight_type_for_node(context, child_node);

    highlight_python_node(context, child_node, child_highlight_type);
  }
}

void
editor_python_highlight_run(
  const uint8_t *source,
  int source_byte_length,
  void (*write_segment)(
    void *writer_context,
    const char *text,
    int text_byte_length,
    uint32_t color
  ),
  void *writer_context
) {

  if (!source || source_byte_length <= 0 || !write_segment)
    return;

  editor_python_highlight_context_t context = {
    .source = source,
    .source_byte_length = source_byte_length,
    .write_segment = write_segment,
    .writer_context = writer_context,
    .last_end_byte_offset = 0,
    .previous_node_type = "",
  };

  TSParser *parser = ts_parser_new();
  TSTree *tree = NULL;

  if (
    parser &&
    ts_parser_set_language(parser, tree_sitter_python())
  ) {

    tree =
      ts_parser_parse_string(
        parser,
        NULL,
        (const char *)source,
        (uint32_t)source_byte_length
      );
  }

  if (tree) {
    TSNode root_node = ts_tree_root_node(tree);

    highlight_python_node(&context, root_node, EDIT_PYTHON_HIGHLIGHT_DEFAULT);
  }

  if (context.last_end_byte_offset < source_byte_length) {
    write_segment(
      writer_context,
      (const char *)(source + context.last_end_byte_offset),
      source_byte_length - context.last_end_byte_offset,
      area512_theme_text_color()
    );
  }

  if (tree)
    ts_tree_delete(tree);

  if (parser)
    ts_parser_delete(parser);
}
