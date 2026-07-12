#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "core/syntax/picoruby/highlight.h"
#include <prism.h>

bool
editor_is_ruby_filename(const char *path, int byte_length) {
  return path && byte_length >= 3 &&
         memcmp(path + byte_length - 3, ".rb", 3) == 0;
}

typedef enum {
  EDIT_HIGHLIGHT_DEFAULT = 0,
  EDIT_HIGHLIGHT_KEYWORD,
  EDIT_HIGHLIGHT_BUILTIN_LITERAL,
  EDIT_HIGHLIGHT_CONSTANT,
  EDIT_HIGHLIGHT_METHOD,
  EDIT_HIGHLIGHT_STRING,
  EDIT_HIGHLIGHT_REGULAR_EXPRESSION,
  EDIT_HIGHLIGHT_SYMBOL,
  EDIT_HIGHLIGHT_NUMBER,
  EDIT_HIGHLIGHT_COMMENT,
  EDIT_HIGHLIGHT_SPECIAL_VARIABLE,
  EDIT_HIGHLIGHT_INTERPOLATION,
} editor_highlight_type_t;

static const uint32_t editor_highlight_colors[] = {
  0,
  0xC586C0,
  0x569CD6,
  0x4EC9B0,
  0xDCDCAA,
  0xCE9178,
  0xD16969,
  0x569CD6,
  0xB5CEA8,
  0x6A9955,
  0x9CDCFE,
  0x569CD6,
};

static const char *const EDITOR_KEYWORD_LIKE_IDENTIFIERS[] = {
  "raise",   "fail",          "require",       "include",       "extend",
  "prepend", "attr",          "attr_accessor", "attr_reader",   "attr_writer",
  "public",  "protected",     "private",       "lambda",        "caller",
  "eval",    "class_eval",    "instance_eval", "module_eval",   "exit",
  "loop",    "define_method", "alias_method",  "remove_method", "undef_method",
};
#define EDITOR_KEYWORD_LIKE_IDENTIFIERS_COUNT                                  \
  (sizeof(EDITOR_KEYWORD_LIKE_IDENTIFIERS) /                                   \
   sizeof(EDITOR_KEYWORD_LIKE_IDENTIFIERS[0]))

static bool
editor_token_text_equals(
  const uint8_t *text,
  int text_byte_length,
  const char *string
) {

  int string_byte_length = (int)strlen(string);

  if (text_byte_length != string_byte_length)
    return false;

  return memcmp(text, string, (size_t)string_byte_length) == 0;
}

static bool
editor_is_keyword_like(const uint8_t *text, int text_byte_length) {
  for (size_t i = 0; i < EDITOR_KEYWORD_LIKE_IDENTIFIERS_COUNT; i++) {
    if (editor_token_text_equals(
          text,
          text_byte_length,
          EDITOR_KEYWORD_LIKE_IDENTIFIERS[i]
        )) {

      return true;
    }
  }

  return false;
}

static editor_highlight_type_t

editor_classify_token(
  editor_highlight_context_t *context,
  pm_token_type_t type,
  const uint8_t *text,
  int text_byte_length
) {

  switch (type) {
  case PM_TOKEN_KEYWORD_DEF:
    context->expect_method_name = true;

    return EDIT_HIGHLIGHT_KEYWORD;

  case PM_TOKEN_STRING_BEGIN:
    context->literal_context = EDIT_LITERAL_STRING;

    return EDIT_HIGHLIGHT_STRING;

  case PM_TOKEN_REGEXP_BEGIN:
    context->literal_context = EDIT_LITERAL_REGULAR_EXPRESSION;

    return EDIT_HIGHLIGHT_REGULAR_EXPRESSION;

  case PM_TOKEN_SYMBOL_BEGIN:
    if (text_byte_length == 1 && text[0] == ':') {
      context->expect_symbol_name = true;
    } else {
      context->literal_context = EDIT_LITERAL_QUOTED_SYMBOL;
    }

    return EDIT_HIGHLIGHT_SYMBOL;

  case PM_TOKEN_STRING_END:
  case PM_TOKEN_LABEL_END: {
    editor_highlight_type_t result_type = EDIT_HIGHLIGHT_STRING;

    if (context->literal_context == EDIT_LITERAL_QUOTED_SYMBOL)
      result_type = EDIT_HIGHLIGHT_SYMBOL;

    context->literal_context = EDIT_LITERAL_NONE;

    return result_type;
  }

  case PM_TOKEN_REGEXP_END:
    context->literal_context = EDIT_LITERAL_NONE;

    return EDIT_HIGHLIGHT_REGULAR_EXPRESSION;

  case PM_TOKEN_STRING_CONTENT:
    switch (context->literal_context) {
    case EDIT_LITERAL_QUOTED_SYMBOL:
      return EDIT_HIGHLIGHT_SYMBOL;

    case EDIT_LITERAL_STRING:
      return EDIT_HIGHLIGHT_STRING;

    case EDIT_LITERAL_REGULAR_EXPRESSION:
      return EDIT_HIGHLIGHT_REGULAR_EXPRESSION;

    default:
      return EDIT_HIGHLIGHT_STRING;
    }

  case PM_TOKEN_CHARACTER_LITERAL:
    return EDIT_HIGHLIGHT_STRING;

  case PM_TOKEN_LABEL:
    return EDIT_HIGHLIGHT_SYMBOL;

  case PM_TOKEN_CONSTANT:
    return EDIT_HIGHLIGHT_CONSTANT;

  case PM_TOKEN_METHOD_NAME:
    context->expect_method_name = false;
    return EDIT_HIGHLIGHT_METHOD;

  case PM_TOKEN_IDENTIFIER:
    if (context->expect_method_name) {
      context->expect_method_name = false;
      return EDIT_HIGHLIGHT_METHOD;
    }

    if (editor_is_keyword_like(text, text_byte_length)) {
      return EDIT_HIGHLIGHT_KEYWORD;
    }

    return EDIT_HIGHLIGHT_DEFAULT;

  case PM_TOKEN_COMMENT:
    return EDIT_HIGHLIGHT_COMMENT;

  case PM_TOKEN_EMBEXPR_BEGIN:
  case PM_TOKEN_EMBEXPR_END:
  case PM_TOKEN_EMBVAR:
    return EDIT_HIGHLIGHT_INTERPOLATION;

  case PM_TOKEN_INSTANCE_VARIABLE:
  case PM_TOKEN_CLASS_VARIABLE:
  case PM_TOKEN_GLOBAL_VARIABLE:
  case PM_TOKEN_BACK_REFERENCE:
  case PM_TOKEN_NUMBERED_REFERENCE:
    return EDIT_HIGHLIGHT_SPECIAL_VARIABLE;

  case PM_TOKEN_INTEGER:
  case PM_TOKEN_FLOAT:
    return EDIT_HIGHLIGHT_NUMBER;

  case PM_TOKEN_KEYWORD_NIL:
  case PM_TOKEN_KEYWORD_TRUE:
  case PM_TOKEN_KEYWORD_FALSE:
  case PM_TOKEN_KEYWORD_SELF:
  case PM_TOKEN_KEYWORD___FILE__:
  case PM_TOKEN_KEYWORD___ENCODING__:
    return EDIT_HIGHLIGHT_BUILTIN_LITERAL;

  default: {
    const char *name = pm_token_type_name(type);

    if (name && strncmp(name, "KEYWORD_", 8) == 0) {
      return EDIT_HIGHLIGHT_KEYWORD;
    }

    return EDIT_HIGHLIGHT_DEFAULT;
  }
  } // switch end
}

static void
write_highlight_segment(
  editor_highlight_context_t *context,
  editor_highlight_type_t type,
  const uint8_t *text,
  int text_byte_length
) {
  if (text_byte_length <= 0)
    return;
  context->write_segment(
    context->writer_context,
    (const char *)text,
    text_byte_length,
    editor_highlight_colors[type]
  );
}

static void
highlight_prism_token(
  void *callback_context,
  pm_parser_t *parser,
  pm_token_t *token
) {
  editor_highlight_context_t *context =
    (editor_highlight_context_t *)callback_context;
  if (token->type == PM_TOKEN_EOF)
    return;
  int start_offset = (int)(token->start - parser->start);
  int byte_length = (int)(token->end - token->start);
  int end_offset = start_offset + byte_length;

  if (start_offset > context->last_end) {
    context->write_segment(
      context->writer_context,
      (const char *)(context->source + context->last_end),
      start_offset - context->last_end,
      0
    );
  }

  const uint8_t *text = context->source + start_offset;

  if (context->expect_symbol_name) {
    context->expect_symbol_name = false;
    write_highlight_segment(context, EDIT_HIGHLIGHT_SYMBOL, text, byte_length);
    context->last_end = end_offset;
    return;
  }

  editor_highlight_type_t highlight_type =
    editor_classify_token(context, token->type, text, byte_length);
  write_highlight_segment(context, highlight_type, text, byte_length);
  context->last_end = end_offset;
}

void
editor_highlight_init(
  editor_highlight_context_t *context,
  const uint8_t *source,
  int source_byte_length,
  void (*write_segment)(
    void *writer_context,
    const char *text,
    int byte_length,
    uint32_t color
  ),
  void *writer_context
) {
  context->source = source;
  context->source_byte_length = source_byte_length;
  context->write_segment = write_segment;
  context->writer_context = writer_context;
  context->last_end = 0;
  context->literal_context = EDIT_LITERAL_NONE;
  context->expect_symbol_name = false;
  context->expect_method_name = false;
}

void
editor_highlight_run(editor_highlight_context_t *context) {
  pm_lex_callback_t highlight_lex_config = {
    .data = context,
    .callback = highlight_prism_token,
  };
  pm_parser_t parser;
  pm_parser_init(
    &parser,
    context->source,
    (size_t)context->source_byte_length,
    NULL
  );
  parser.lex_callback = &highlight_lex_config;
  pm_node_t *node = pm_parse(&parser);
  pm_node_destroy(&parser, node);
  pm_parser_free(&parser);

  if (context->last_end < context->source_byte_length) {
    context->write_segment(
      context->writer_context,
      (const char *)(context->source + context->last_end),
      context->source_byte_length - context->last_end,
      0
    );
  }
}
