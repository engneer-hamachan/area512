#include "core/syntax/picoruby/indent.h"
#include <prism.h>
#include <stddef.h>
#include <stdint.h>

#define AUTO_INDENT_MAX_TOKENS 96

typedef struct {
  pm_token_type_t type[AUTO_INDENT_MAX_TOKENS];
  int count;
} auto_indent_tokens;

static void
collect_auto_indent_token(
  void *callback_context,
  pm_parser_t *parser,
  pm_token_t *token
) {

  (void)parser;

  auto_indent_tokens *tokens = (auto_indent_tokens *)callback_context;

  switch (token->type) {
  case PM_TOKEN_EOF:
  case PM_TOKEN_COMMENT:
  case PM_TOKEN_NEWLINE:
    return;

  default:
    break;
  }

  if (tokens->count < AUTO_INDENT_MAX_TOKENS)
    tokens->type[tokens->count++] = token->type;
}

static void
auto_indent_tokenize(
  auto_indent_tokens *tokens,
  const char *line,
  int byte_length
) {

  tokens->count = 0;

  if (!line || byte_length <= 0)
    return;

  pm_lex_callback_t auto_indent_lex_config = {
    .data = tokens,
    .callback = collect_auto_indent_token
  };

  pm_parser_t parser;
  pm_parser_init(&parser, (const uint8_t *)line, (size_t)byte_length, NULL);

  parser.lex_callback = &auto_indent_lex_config;
  pm_node_t *node = pm_parse(&parser);

  pm_node_destroy(&parser, node);
  pm_parser_free(&parser);
}

static int
auto_indent_is_increase_keyword(pm_token_type_t type) {
  switch (type) {
  case PM_TOKEN_KEYWORD_BEGIN:
  case PM_TOKEN_KEYWORD_CLASS:
  case PM_TOKEN_KEYWORD_MODULE:
  case PM_TOKEN_KEYWORD_DEF:
  case PM_TOKEN_KEYWORD_IF:
  case PM_TOKEN_KEYWORD_UNLESS:
  case PM_TOKEN_KEYWORD_ELSIF:
  case PM_TOKEN_KEYWORD_ELSE:
  case PM_TOKEN_KEYWORD_RESCUE:
  case PM_TOKEN_KEYWORD_ENSURE:
  case PM_TOKEN_KEYWORD_CASE:
  case PM_TOKEN_KEYWORD_WHEN:
  case PM_TOKEN_KEYWORD_IN:
  case PM_TOKEN_KEYWORD_WHILE:
  case PM_TOKEN_KEYWORD_UNTIL:
  case PM_TOKEN_KEYWORD_FOR:
    return 1;
  default:
    return 0;
  }
}

static int
auto_indent_is_continuation_type(pm_token_type_t type) {
  switch (type) {
  case PM_TOKEN_DOT:
  case PM_TOKEN_COMMA:
  case PM_TOKEN_LABEL:
  case PM_TOKEN_STAR:
  case PM_TOKEN_SLASH:
  case PM_TOKEN_PERCENT:
  case PM_TOKEN_PLUS:
  case PM_TOKEN_MINUS:
  case PM_TOKEN_EQUAL:
  case PM_TOKEN_PIPE:
  case PM_TOKEN_AMPERSAND:
  case PM_TOKEN_QUESTION_MARK:
  case PM_TOKEN_AMPERSAND_AMPERSAND:
  case PM_TOKEN_PIPE_PIPE:
  case PM_TOKEN_KEYWORD_AND:
  case PM_TOKEN_KEYWORD_OR:
    return 1;
  default:
    return 0;
  }
}

static int
auto_indent_is_block_or_brace_opener(const auto_indent_tokens *tokens) {
  int count = tokens->count;

  if (count == 0)
    return 0;

  if (tokens->type[count - 1] == PM_TOKEN_PIPE) {
    int i = count - 2;
    while (i >= 0 && tokens->type[i] != PM_TOKEN_PIPE)
      i--;
    if (i >= 0)
      count = i;
    else
      count--;
  }

  if (count == 0)
    return 0;

  pm_token_type_t last = tokens->type[count - 1];

  return (last == PM_TOKEN_BRACE_LEFT || last == PM_TOKEN_KEYWORD_DO);
}

static int
auto_indent_is_delimiter_opener(const auto_indent_tokens *tokens) {
  if (tokens->count == 0)
    return 0;

  pm_token_type_t last = tokens->type[tokens->count - 1];

  return (
    last == PM_TOKEN_PARENTHESIS_LEFT || last == PM_TOKEN_BRACKET_LEFT_ARRAY
  );
}

static int
auto_indent_is_continuation(const auto_indent_tokens *tokens) {
  if (tokens->count == 0)
    return 0;

  return auto_indent_is_continuation_type(tokens->type[tokens->count - 1]);
}

static int
auto_indent_is_endless_def(const auto_indent_tokens *tokens) {
  int def_index = -1;

  for (int i = 0; i < tokens->count; i++) {
    if (tokens->type[i] == PM_TOKEN_KEYWORD_DEF)
      def_index = i;
  }

  if (def_index < 0)
    return 0;

  for (int i = def_index + 1; i < tokens->count; i++) {
    if (tokens->type[i] == PM_TOKEN_EQUAL)
      return 1;
  }

  return 0;
}

int
editor_auto_indent_should_increase(
  const char *current_line,
  int current_byte_length,
  const char *previous_line,
  int previous_byte_length
) {

  auto_indent_tokens current_tokens, previous_tokens;

  auto_indent_tokenize(&current_tokens, current_line, current_byte_length);
  auto_indent_tokenize(&previous_tokens, previous_line, previous_byte_length);

  pm_token_type_t first = PM_TOKEN_EOF;

  if (current_tokens.count > 0)
    first = current_tokens.type[0];

  if (auto_indent_is_increase_keyword(first) &&
      !(first == PM_TOKEN_KEYWORD_DEF &&
        auto_indent_is_endless_def(&current_tokens))) {

    return 1;
  }

  if (auto_indent_is_block_or_brace_opener(&current_tokens))
    return 1;

  if (auto_indent_is_delimiter_opener(&current_tokens))
    return 1;

  if (auto_indent_is_continuation(&current_tokens) &&
      !auto_indent_is_continuation(&previous_tokens) &&
      !auto_indent_is_block_or_brace_opener(&previous_tokens)) {

    return 1;
  }

  return 0;
}

int
editor_auto_indent_should_decrease(const char *line, int line_byte_length) {
  auto_indent_tokens tokens;

  auto_indent_tokenize(&tokens, line, line_byte_length);

  if (tokens.count == 0)
    return 0;

  switch (tokens.type[0]) {
  case PM_TOKEN_KEYWORD_END:
  case PM_TOKEN_KEYWORD_ELSE:
  case PM_TOKEN_KEYWORD_ELSIF:
  case PM_TOKEN_KEYWORD_WHEN:
  case PM_TOKEN_KEYWORD_RESCUE:
  case PM_TOKEN_KEYWORD_ENSURE:
  case PM_TOKEN_KEYWORD_IN:
    return 1;
  default:
    return 0;
  }
}
