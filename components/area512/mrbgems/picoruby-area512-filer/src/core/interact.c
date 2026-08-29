#if defined(PICORB_VM_MRUBYC)

#include "core/filer.h"
#include "core/terminal/terminal.h"

#include <stdio.h>
#include <string.h>

static void
set_message(Filer *filer, const char *text) {
  strncpy(filer->message, text, MESSAGE_MAX - 1);
}

static int
confirm_delete(Filer *filer, FileEntry *entry) {
  char question[MESSAGE_MAX];

  build_delete_question(
    filer,
    entry->name,
    entry->type == ENTRY_TYPE_DIR,
    question,
    sizeof question
  );

  return read_yes_no_confirmation(filer, question);
}

// Interaction loop; returns an action code when Ruby work is needed. Single
// exit so c_filer_run can wrap the whole loop in one foreground input
// session.
int
run_filer_interaction(Filer *filer) {
  for (;;) {
    if (filer->terminal) {
      int action = run_terminal_session(filer);

      if (action == TERMINAL_ACTION_EXIT) {
        close_terminal_session(filer);
      } else {
        area512_filer_teardown_ui(filer);

        return action;
      }
    }

    draw_all(filer);

    int key = area512_filer_read_key();

    FileEntry *entry = fetch_selected_entry(filer);

    switch (key) {
    case KEY_DOWN:
      move_cursor(filer, 1);

      break;

    case KEY_UP:
      move_cursor(filer, -1);

      break;

    case KEY_LEFT:
      return ACTION_UP; // navigation: keep UI up to avoid flicker

    case KEY_RIGHT:
      if (!entry)
        break;

      if (entry->type == ENTRY_TYPE_UP)
        return ACTION_UP;

      if (entry->type == ENTRY_TYPE_DIR)
        return ACTION_OPEN_DIR;

      if (is_selected_markdown_file(filer)) {
        area512_filer_teardown_ui(filer);
        return ACTION_VIEW_MARKDOWN;
      }

      if (is_selected_ruby_file(filer)) {
        area512_filer_teardown_ui(filer);
        return ACTION_RUN_RUBY;
      }

      if (is_selected_python_file(filer)) {
        area512_filer_teardown_ui(filer);
        return ACTION_RUN_PYTHON;
      }

      if (is_selected_dot_image_file(filer)) {
        area512_filer_teardown_ui(filer);
        return ACTION_EDIT_DOT;
      }

      set_message(filer, "Not runnable");

      break;

    case KEY_COMPILE:
      if (is_selected_source_file(filer)) {
        area512_filer_teardown_ui(filer);

        return ACTION_COMPILE;
      }

      set_message(filer, "No source file");

      break;

    case KEY_COMPILE_ALL:
      area512_filer_teardown_ui(filer);

      return ACTION_COMPILE_ALL;

    case KEY_EDIT:
      if (is_selected_editable(filer)) {
        area512_filer_teardown_ui(filer);

        return ACTION_EDIT;
      }

      set_message(filer, "Not editable");

      break;

    case KEY_RUN_DIR:
      if (entry && entry->type == ENTRY_TYPE_DIR) {
        area512_filer_teardown_ui(filer);

        return ACTION_RUN_DIR;
      }

      set_message(filer, "Select a directory");

      break;

    case KEY_DELETE:
      if (!entry)
        break;

      if (entry->type == ENTRY_TYPE_UP) {
        set_message(filer, "Cannot delete ..");
        break;
      }

      if (confirm_delete(filer, entry))
        return ACTION_DELETE;

      break;

    case KEY_NEW_FILE:
      if (read_text_input(filer, "New file: "))
        return ACTION_NEW_FILE;

      break;

    case KEY_NEW_DIR:
      if (read_text_input(filer, "New dir: "))
        return ACTION_NEW_DIR;

      break;

    case KEY_MOVE:
      if (!entry)
        break;

      if (entry->type == ENTRY_TYPE_UP) {
        set_message(filer, "Cannot move ..");
        break;
      }

      if (read_text_input(filer, "Move to: "))
        return ACTION_MOVE;

      break;

    case KEY_COPY:
      if (!entry)
        break;

      if (entry->type == ENTRY_TYPE_UP) {
        set_message(filer, "Cannot copy ..");
        break;
      }

      if (read_text_input(filer, "Copy to: "))
        return ACTION_COPY;

      break;

    case KEY_TERMINAL:
      open_terminal_session(filer);

      break;

    case KEY_REBOOT:
      area512_filer_teardown_ui(filer);

      return ACTION_REBOOT;

    default:
      if (key >= '1' && key <= '9')
        jump_to(filer, key - '1');

      break;
    }
  }
}

#endif
