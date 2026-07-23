#if defined(PICORB_VM_MRUBYC)

#include "area512_markdown.h"

#include "core/markdown/viewer.h"
#include "port/area512_editor_canvas.h"
#include "port/area512_editor_file.h"
#include "port/area512_editor_host.h"

#include "area512_hal.h"
#include <stdbool.h>
#include "io-console.h"
#include <mrubyc.h>
#include <stdlib.h>

typedef struct {
  MarkdownViewer *viewer;
  Area512EditorCanvas canvas;
} markdown_session;

static markdown_session *
get_session(mrbc_value *values) {
  return *(markdown_session **)values->instance->data;
}

static void
c_markdown_new(
  mrbc_vm *virtual_machine,
  mrbc_value *values,
  int argument_count
) {

  markdown_session *session =
    (markdown_session *)malloc(sizeof(markdown_session));

  if (!session) {
    mrbc_raise(
      virtual_machine,
      MRBC_CLASS(RuntimeError),
      "failed to allocate Markdown"
    );

    return;
  }

  session->viewer = NULL;
  session->canvas.row_sprite = NULL;
  session->canvas.cursor_sprite = NULL;

  MarkdownViewer *viewer = (MarkdownViewer *)malloc(sizeof(MarkdownViewer));

  if (!viewer) {
    free(session);

    mrbc_raise(
      virtual_machine,
      MRBC_CLASS(RuntimeError),
      "failed to allocate Markdown"
    );

    return;
  }

  session->viewer = viewer;

  int columns = area512_gfx_width() / EDIT_CHAR_WIDTH;
  int rows = area512_gfx_height() / EDIT_ROW_HEIGHT;

  if (columns < 1)
    columns = 1;

  if (rows < 1)
    rows = 1;

  init_markdown_viewer(viewer, columns, rows);

  if (argument_count >= 1 && mrbc_type(values[1]) == MRBC_TT_STRING &&
      values[1].string->size > 0) {

    const char *path = (const char *)values[1].string->data;
    int path_byte_length = values[1].string->size;

    set_markdown_viewer_filepath(viewer, path, path_byte_length);

    VimString content;
    vim_string_init(&content);

    if (load_edit_file(path, path_byte_length, &content))
      load_markdown_viewer_text(viewer, content.bytes, content.byte_length);

    vim_string_free(&content);
  }

  mrbc_value instance =
    mrbc_instance_new(virtual_machine, values->cls, sizeof(markdown_session *));

  *(markdown_session **)instance.instance->data = session;

  mrbc_decref(values);

  values[0] = instance;
}

static void
c_markdown_show(
  mrbc_vm *virtual_machine,
  mrbc_value *values,
  int argument_count
) {

  (void)virtual_machine;
  (void)argument_count;

  markdown_session *session = get_session(values);

  if (!session || !session->viewer) {
    mrbc_decref(values);
    mrbc_set_nil(values);
    return;
  }

  MarkdownViewer *viewer = session->viewer;

  session->canvas.char_width = EDIT_CHAR_WIDTH;
  session->canvas.row_height = EDIT_ROW_HEIGHT;
  session->canvas.font_size = EDIT_BODY_FONT_SIZE;

  io_raw_bang(false);
  area512_gfx_fill_screen(EDIT_BACKGROUND);

  session->canvas.row_sprite = area512_sprite_new_with_font_size(
    area512_gfx_width(),
    editor_canvas_font_row_height(EDIT_HEADING1_FONT_SIZE),
    EDIT_BODY_FONT_SIZE
  );

  VimCanvas canvas = {
    .context = &session->canvas,
    .clear_row = clear_editor_canvas_row,
    .draw_row_text = draw_editor_canvas_row_text,
    .push_row = push_editor_canvas_row,
    .set_font_size = set_editor_canvas_font_size,
    .draw_cursor = NULL,
  };

  draw_markdown_viewer(viewer, &canvas);

  for (;;) {
    int first_byte = area512_console_getch_block();

    if (first_byte < 0)
      continue;

    MarkdownViewerStatus status;

    if (first_byte == 27) {
      char sequence[2];
      int sequence_byte_length = read_escape_sequence(sequence);

      status =
        handle_markdown_viewer_escape(viewer, sequence, sequence_byte_length);

    } else {
      status = handle_markdown_viewer_key(viewer, first_byte);
    }

    if (status == MARKDOWN_VIEWER_QUIT)
      break;

    draw_markdown_viewer(viewer, &canvas);
  }

  if (session->canvas.row_sprite) {
    area512_sprite_delete(session->canvas.row_sprite);
    session->canvas.row_sprite = NULL;
  }

  io_cooked_bang();

  mrbc_decref(values);
  mrbc_set_nil(values);
}

static void
mrbc_markdown_free(mrbc_value *self) {
  markdown_session *session = *(markdown_session **)self->instance->data;

  if (!session)
    return;

  if (session->canvas.row_sprite)
    area512_sprite_delete(session->canvas.row_sprite);

  if (session->viewer) {
    free_markdown_viewer(session->viewer);
    free(session->viewer);
  }

  free(session);
}

void
define_markdown_class(mrbc_vm *virtual_machine) {
  mrbc_class *markdown_class =
    mrbc_define_class(virtual_machine, "Markdown", mrbc_class_object);

  mrbc_define_destructor(markdown_class, mrbc_markdown_free);
  mrbc_define_method(virtual_machine, markdown_class, "new", c_markdown_new);
  mrbc_define_method(virtual_machine, markdown_class, "show", c_markdown_show);
}

#endif
