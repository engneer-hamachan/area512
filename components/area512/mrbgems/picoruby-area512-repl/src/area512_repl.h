#pragma once

#define REPL_KEY_LEFT 1001
#define REPL_KEY_RIGHT 1002

#define REPL_INPUT_SUBMITTED 0
#define REPL_INPUT_ESCAPED 1
#define REPL_INPUT_INTERRUPTED 2
#define REPL_INPUT_OUT_OF_MEMORY 3

typedef struct {
  char *input_line;
  int input_byte_count;
  int input_byte_capacity;
  int input_cursor_byte_offset;
} ReplLine;

int open_repl_line(ReplLine *repl_line);
void close_repl_line(ReplLine *repl_line);
int read_repl_input_line(const char *prompt, ReplLine *repl_line);

void *create_repl_row_sprite(void);
void draw_repl_input_line(
  void *row_sprite,
  const char *prompt,
  const ReplLine *repl_line
);
