#if defined(PICORB_VM_MRUBYC)

#include "core/terminal/terminal.h"

#include <stdio.h>
#include <string.h>

#define COMMAND_WORD_CAPACITY 3

typedef struct {
  char command_line_text[LINE_MAX];
  const char *command_name;
  char first_argument[LINE_MAX];
  char second_argument[LINE_MAX];
  int argument_count;
} CommandLine;

static const char *const COMMAND_NAME_TABLE[] = {
  "cd",
  "run",
  "md",
  "dot",
  "vim",
  "compile",
  "touch",
  "mkdir",
  "rm",
  "mv",
  "cp",
  "reboot",
  "ls",
  "pwd",
  "top",
  "exit",
  "clear",
  "help",
  "irb"
};

#define COMMAND_NAME_COUNT \
  ((int)(sizeof COMMAND_NAME_TABLE / sizeof COMMAND_NAME_TABLE[0]))

const char *
fetch_command_name(int command_index) {
  if (command_index < 0 || command_index >= COMMAND_NAME_COUNT)
    return NULL;

  return COMMAND_NAME_TABLE[command_index];
}

static void
parse_command_line(const char *input_line, CommandLine *command_line) {
  char *command_line_cursor = command_line->command_line_text;
  const char *unexpanded_first_argument = "";
  const char *unexpanded_second_argument = "";
  int word_index = 0;

  strncpy(command_line->command_line_text, input_line, LINE_MAX - 1);
  command_line->command_line_text[LINE_MAX - 1] = 0;

  command_line->command_name = "";

  while (*command_line_cursor && word_index < COMMAND_WORD_CAPACITY) {
    while (*command_line_cursor == ' ')
      *command_line_cursor++ = 0;

    if (*command_line_cursor == 0)
      break;

    if (word_index == 0)
      command_line->command_name = command_line_cursor;
    else if (word_index == 1)
      unexpanded_first_argument = command_line_cursor;
    else
      unexpanded_second_argument = command_line_cursor;

    word_index++;

    while (*command_line_cursor && *command_line_cursor != ' ')
      command_line_cursor++;
  }

  expand_tilde(
    unexpanded_first_argument,
    command_line->first_argument,
    LINE_MAX
  );

  expand_tilde(
    unexpanded_second_argument,
    command_line->second_argument,
    LINE_MAX
  );

  command_line->argument_count = word_index > 0 ? word_index - 1 : 0;
}

static int
is_builtin_command(const char *command_name) {
  return strcmp(command_name, "ls") == 0
      || strcmp(command_name, "pwd") == 0
      || strcmp(command_name, "top") == 0
      || strcmp(command_name, "clear") == 0
      || strcmp(command_name, "help") == 0
      || strcmp(command_name, "irb") == 0
      || strcmp(command_name, "exit") == 0;
}

static void
append_terminal_command_names(Filer *filer) {
  char command_name_list_text[LINE_MAX];
  int command_index = 0;

  command_name_list_text[0] = 0;

  while (command_index < COMMAND_NAME_COUNT) {
    snprintf(
      command_name_list_text + strlen(command_name_list_text),
      sizeof command_name_list_text - strlen(command_name_list_text),
      "%s ",
      COMMAND_NAME_TABLE[command_index]
    );

    command_index++;
  }

  append_output_text(filer->terminal, command_name_list_text);
}

static int
execute_builtin_terminal_command(
  Filer *filer,
  const CommandLine *command_line
) {

  if (strcmp(command_line->command_name, "exit") == 0)
    return TERMINAL_ACTION_EXIT;

  if (strcmp(command_line->command_name, "irb") == 0)
    return ACTION_IRB;

  if (strcmp(command_line->command_name, "ls") == 0) {
    if (command_line->argument_count < 1)
      show_terminal_entries(filer);
    else
      show_directory_entries(filer, command_line->first_argument);

  } else if (strcmp(command_line->command_name, "pwd") == 0) {
    append_output_text(filer->terminal, filer->current_directory);
  } else if (strcmp(command_line->command_name, "top") == 0) {
    show_metrics(filer);
  } else if (strcmp(command_line->command_name, "clear") == 0) {
    clear_output_lines(filer->terminal);

    filer->full_redraw = 1;

  } else if (strcmp(command_line->command_name, "help") == 0) {
    append_terminal_command_names(filer);
  }

  return ACTION_NONE;
}

static int
read_terminal_yes_no_confirmation(Filer *filer, const char *question) {
  int input_key;

  append_output_text(filer->terminal, question);

  for (;;) {
    draw_terminal(filer);

    input_key = read_terminal_key();

    if (input_key == 'y' || input_key == 'Y')
      return 1;

    if (input_key == 'n' || input_key == 'N' || input_key == 27)
      return 0;
  }
}

static int
select_entry_by_name(Filer *filer, const char *entry_name) {
  int entry_index = 0;

  while (entry_index < filer->count) {
    if (
      filer->entries[entry_index].type != ENTRY_TYPE_UP &&
      strcmp(filer->entries[entry_index].name, entry_name) == 0
    ) {

      filer->index = entry_index;

      return 1;
    }

    entry_index++;
  }

  return 0;
}

static int
has_path_separator(const char *input_path) {
  return strchr(input_path, '/') != NULL;
}

static int
select_command_target(
  Filer *filer,
  const char *input_path,
  int *target_is_directory
) {

  FileEntry *entry;

  if (has_path_separator(input_path))
    return resolve_target_path(
      filer->current_directory,
      input_path,
      filer->action_target_path,
      CURRENT_DIRECTORY_MAX,
      target_is_directory
    );

  if (!select_entry_by_name(filer, input_path))
    return TARGET_MISSING;

  entry = fetch_selected_entry(filer);
  *target_is_directory = (entry->type == ENTRY_TYPE_DIR);

  return TARGET_RESOLVED;
}

static void
append_target_error_message(
  Filer *filer,
  int select_status,
  const char *input_path
) {

  char message_text[MESSAGE_MAX];

  if (select_status == TARGET_INVALID) {
    append_output_text(filer->terminal, "Bad path");

    return;
  }

  snprintf(message_text, sizeof message_text, "No such file: %s", input_path);

  append_output_text(filer->terminal, message_text);
}

static int
prepare_missing_file_edit_action(Filer *filer, const char *input_path) {
  char message_text[MESSAGE_MAX];

  if (!is_editable_file_path(input_path)) {
    snprintf(message_text, sizeof message_text, "Cannot vim: %s", input_path);

    append_output_text(filer->terminal, message_text);

    return ACTION_NONE;
  }

  if (!parent_path_is_directory(filer->action_target_path)) {
    snprintf(
      message_text,
      sizeof message_text,
      "No such directory: %s",
      input_path
    );

    append_output_text(filer->terminal, message_text);

    return ACTION_NONE;
  }

  return ACTION_EDIT;
}

static int
prepare_filer_action(Filer *filer, const CommandLine *command_line) {
  char message_text[MESSAGE_MAX];
  char question_text[MESSAGE_MAX];
  int select_status;
  int target_is_directory = 0;

  if (strcmp(command_line->command_name, "reboot") == 0)
    return ACTION_REBOOT;

  if (
    strcmp(command_line->command_name, "compile") == 0 &&
    strcmp(command_line->first_argument, "--all") == 0
  ) {

    if (command_line->argument_count < 2)
      return ACTION_COMPILE_ALL;

    select_status =
      resolve_target_path(
        filer->current_directory,
        command_line->second_argument,
        filer->action_target_path,
        CURRENT_DIRECTORY_MAX,
        &target_is_directory
      );

    if (select_status != TARGET_RESOLVED) {
      append_target_error_message(
        filer,
        select_status,
        command_line->second_argument
      );

      return ACTION_NONE;
    }

    if (!target_is_directory) {
      snprintf(
        message_text,
        sizeof message_text,
        "Cannot compile: %s",
        command_line->second_argument
      );

      append_output_text(filer->terminal, message_text);

      return ACTION_NONE;
    }

    return ACTION_COMPILE_ALL;
  }

  if (
    strcmp(command_line->command_name, "mv") == 0 ||
    strcmp(command_line->command_name, "cp") == 0
  ) {

    if (command_line->argument_count < 2) {
      snprintf(
        message_text,
        sizeof message_text,
        "Usage: %s <src> <dst>",
        command_line->command_name
      );

      append_output_text(filer->terminal, message_text);

      return ACTION_NONE;
    }

    select_status =
      select_command_target(
        filer,
        command_line->first_argument,
        &target_is_directory
      );

    if (select_status != TARGET_RESOLVED) {
      append_target_error_message(
        filer,
        select_status,
        command_line->first_argument
      );

      return ACTION_NONE;
    }

    strncpy(filer->input, command_line->second_argument, LINE_MAX - 1);
    filer->input[LINE_MAX - 1] = 0;

    if (strcmp(command_line->command_name, "mv") == 0)
      return ACTION_MOVE;

    return ACTION_COPY;
  }

  if (strcmp(command_line->command_name, "cd") == 0) {
    strncpy(
      filer->input,
      command_line->argument_count < 1
        ? TERMINAL_HOME_DIRECTORY
        : command_line->first_argument,
      LINE_MAX - 1
    );

    filer->input[LINE_MAX - 1] = 0;

    return ACTION_CHANGE_DIR;
  }

  if (
    strcmp(command_line->command_name, "touch") == 0 ||
    strcmp(command_line->command_name, "mkdir") == 0
  ) {

    if (command_line->argument_count < 1) {
      snprintf(
        message_text,
        sizeof message_text,
        "Usage: %s <name>",
        command_line->command_name
      );

      append_output_text(filer->terminal, message_text);

      return ACTION_NONE;
    }

    strncpy(filer->input, command_line->first_argument, LINE_MAX - 1);
    filer->input[LINE_MAX - 1] = 0;

    if (strcmp(command_line->command_name, "touch") == 0)
      return ACTION_NEW_FILE;

    return ACTION_NEW_DIR;
  }

  if (
    strcmp(command_line->command_name, "run") == 0 ||
    strcmp(command_line->command_name, "md") == 0 ||
    strcmp(command_line->command_name, "dot") == 0 ||
    strcmp(command_line->command_name, "vim") == 0 ||
    strcmp(command_line->command_name, "compile") == 0 ||
    strcmp(command_line->command_name, "rm") == 0
  ) {

    if (command_line->argument_count < 1) {
      snprintf(
        message_text,
        sizeof message_text,
        "Usage: %s <name>",
        command_line->command_name
      );

      append_output_text(filer->terminal, message_text);

      return ACTION_NONE;
    }

    select_status =
      select_command_target(
        filer,
        command_line->first_argument,
        &target_is_directory
      );

    if (
      select_status == TARGET_MISSING &&
      strcmp(command_line->command_name, "vim") == 0
    ) {

      select_status =
        resolve_target_path(
          filer->current_directory,
          command_line->first_argument,
          filer->action_target_path,
          CURRENT_DIRECTORY_MAX,
          &target_is_directory
        );

      if (select_status == TARGET_MISSING)
        return prepare_missing_file_edit_action(
                 filer,
                 command_line->first_argument
               );
    }

    if (select_status != TARGET_RESOLVED) {
      append_target_error_message(
        filer,
        select_status,
        command_line->first_argument
      );

      return ACTION_NONE;
    }

    if (strcmp(command_line->command_name, "rm") == 0) {
      build_delete_question(
        filer,
        command_line->first_argument,
        target_is_directory,
        question_text,
        sizeof question_text
      );

      return read_terminal_yes_no_confirmation(filer, question_text)
        ? ACTION_DELETE
        : ACTION_NONE;
    }

    if (strcmp(command_line->command_name, "run") == 0 && target_is_directory)
      return ACTION_RUN_DIR;

    if (!target_is_directory) {
      if (
        strcmp(command_line->command_name, "run") == 0 &&
        is_ruby_file_path(command_line->first_argument)
      )
        return ACTION_RUN_RUBY;

      if (
        strcmp(command_line->command_name, "run") == 0 &&
        is_python_file_path(command_line->first_argument)
      )
        return ACTION_RUN_PYTHON;

      if (
        strcmp(command_line->command_name, "md") == 0 &&
        is_markdown_file_path(command_line->first_argument)
      )
        return ACTION_VIEW_MARKDOWN;

      if (
        strcmp(command_line->command_name, "dot") == 0 &&
        is_dot_image_file_path(command_line->first_argument)
      )
        return ACTION_EDIT_DOT;

      if (
        strcmp(command_line->command_name, "vim") == 0 &&
        is_editable_file_path(command_line->first_argument)
      )
        return ACTION_EDIT;

      if (
        strcmp(command_line->command_name, "compile") == 0 &&
        is_source_file_path(command_line->first_argument)
      )
        return ACTION_COMPILE;
    }

    snprintf(
      message_text,
      sizeof message_text,
      "Cannot %s: %s",
      command_line->command_name,
      command_line->first_argument
    );

    append_output_text(filer->terminal, message_text);

    return ACTION_NONE;
  }

  snprintf(
    message_text,
    sizeof message_text,
    "Unknown command: %s",
    command_line->command_name
  );

  append_output_text(filer->terminal, message_text);

  return ACTION_NONE;
}

int
execute_terminal_command(Filer *filer) {
  CommandLine command_line;

  filer->action_target_path[0] = 0;

  parse_command_line(filer->terminal->input_line, &command_line);

  if (is_builtin_command(command_line.command_name))
    return execute_builtin_terminal_command(filer, &command_line);

  return prepare_filer_action(filer, &command_line);
}

#endif
