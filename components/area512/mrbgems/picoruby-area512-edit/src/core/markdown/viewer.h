#ifndef EDIT_MARKDOWN_VIEWER_H
#define EDIT_MARKDOWN_VIEWER_H

#include <stdint.h>

#include "core/markdown/parse.h"
#include "core/render/screen.h"

typedef enum {
  MARKDOWN_VIEWER_CONTINUE = 0,
  MARKDOWN_VIEWER_QUIT
} MarkdownViewerStatus;

typedef struct {
  VimBuffer buffer;

  // One MarkdownCodeLanguage per source line, MARKDOWN_CODE_NONE outside a
  // fence: fenced code needs the state of every line above.
  uint8_t *code_language;

  VimString filepath;

  int width, height;
  int footer_height;
  int top_line;
} MarkdownViewer;

void init_markdown_viewer(MarkdownViewer *viewer, int width, int height);
void free_markdown_viewer(MarkdownViewer *viewer);

void set_markdown_viewer_filepath(
  MarkdownViewer *viewer,
  const char *text,
  int byte_length
);

void load_markdown_viewer_text(
  MarkdownViewer *viewer,
  const char *text,
  int byte_length
);

void draw_markdown_viewer(MarkdownViewer *viewer, VimCanvas *canvas);

MarkdownViewerStatus handle_markdown_viewer_key(
  MarkdownViewer *viewer,
  int key_byte
);

MarkdownViewerStatus handle_markdown_viewer_escape(
  MarkdownViewer *viewer,
  const char *sequence,
  int byte_length
);

#endif
