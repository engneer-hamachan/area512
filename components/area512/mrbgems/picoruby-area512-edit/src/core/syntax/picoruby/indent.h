#ifndef EDIT_SYNTAX_PICORUBY_INDENT_H
#define EDIT_SYNTAX_PICORUBY_INDENT_H

#ifdef __cplusplus
extern "C" {
#endif

int editor_auto_indent_should_increase(
  const char *current_line,
  int current_byte_length,
  const char *previous_line,
  int previous_byte_length
);
int editor_auto_indent_should_decrease(const char *line, int line_byte_length);

#ifdef __cplusplus
}
#endif

#endif
