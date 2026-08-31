#include "area512_hal.h"

#include <stdio.h>
#include <string.h>

#define DEFAULT_UI_PATH "etc/ui"
#define DEFAULT_UI_LINE_CAPACITY 64

static int s_default_ui_is_terminal = 0;

static int
read_text_line(FILE *file, char *line, size_t line_capacity) {
  int byte_count = 0;
  int character;

  while ((character = fgetc(file)) != EOF) {
    if (character == '\n')
      break;

    if (character != '\r' && (size_t)byte_count + 1 < line_capacity)
      line[byte_count++] = (char)character;
  }

  line[byte_count] = '\0';

  return byte_count > 0 || character != EOF;
}

static void
assign_default_ui_from_line(const char *line) {
  if (strcmp(line, "default=terminal") == 0)
    s_default_ui_is_terminal = 1;
  else if (strcmp(line, "default=graphical") == 0)
    s_default_ui_is_terminal = 0;
}

void
area512_default_ui_load(void) {
  char resolved_path[AREA512_PATH_MAX];

  if (
    area512_resolve_data_path(
      DEFAULT_UI_PATH,
      resolved_path,
      sizeof(resolved_path)
    ) < 0
  )
    return;

  FILE *file = fopen(resolved_path, "r");

  if (file == NULL)
    return;

  char line[DEFAULT_UI_LINE_CAPACITY];

  while (read_text_line(file, line, sizeof(line)))
    assign_default_ui_from_line(line);

  fclose(file);
}

int
area512_default_ui_is_terminal(void) {
  return s_default_ui_is_terminal;
}
