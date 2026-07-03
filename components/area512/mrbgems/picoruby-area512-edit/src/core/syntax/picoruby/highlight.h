#ifndef EDIT_SYNTAX_PICORUBY_HIGHLIGHT_H
#define EDIT_SYNTAX_PICORUBY_HIGHLIGHT_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  EDIT_LITERAL_NONE = 0,
  EDIT_LITERAL_STRING,
  EDIT_LITERAL_REGULAR_EXPRESSION,
  EDIT_LITERAL_QUOTED_SYMBOL,
} editor_literal_context_t;

typedef struct {
  const uint8_t *source;
  int source_byte_length;
  void (*write_segment)(
    void *writer_context,
    const char *text,
    int byte_length,
    uint32_t color
  );
  void *writer_context;
  int last_end;
  editor_literal_context_t literal_context;
  bool expect_symbol_name;
  bool expect_method_name;
} editor_highlight_context_t;

bool editor_is_ruby_filename(const char *path, int byte_length);

void editor_highlight_init(
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
);

void editor_highlight_run(editor_highlight_context_t *context);

#ifdef __cplusplus
}
#endif

#endif
