#if defined(PICORB_VM_MRUBYC)

#include "core/filer.h"

#include <stdio.h>
#include <string.h>

static void
set_message(Filer *filer, const char *text) {
  strncpy(filer->message, text, MESSAGE_MAX - 1);
}

static int
confirm_delete(Filer *filer, FileEntry *entry) {
  char short_name[NAME_MAX];
  char question[MESSAGE_MAX];

  fit_string(short_name, sizeof short_name, entry->name, filer->columns - 22);

  if (entry->type == ENTRY_TYPE_DIR)
    snprintf(
      question,
      sizeof question,
      "Delete %s/ & contents? (y/n)",
      short_name
    );
  else
    snprintf(question, sizeof question, "Delete %s? (y/n)", short_name);

  return read_yes_no_confirmation(filer, question);
}

// Interaction loop; returns an action code when Ruby work is needed. Single
// exit so c_filer_run can wrap the whole loop in one foreground input
// session.
int
run_filer_interaction(Filer *filer) {
  for (;;) {
    draw_all(filer);

    int key = area512_filer_read_key();

    FileEntry *entry = filer->count > 0 ? &filer->entries[filer->index] : NULL;

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

      if (entry->type == ENTRY_TYPE_APP) {
        area512_filer_teardown_ui(filer);
        return entry->has_mrb ? ACTION_RUN_MRB : ACTION_COMPILE;
      }

      if (is_selected_markdown_file(filer)) {
        area512_filer_teardown_ui(filer);
        return ACTION_VIEW_MARKDOWN;
      }

      set_message(filer, "Not runnable");

      break;

    case KEY_COMPILE:
      if (entry && entry->type == ENTRY_TYPE_APP && entry->has_rb) {
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
