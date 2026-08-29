#if defined(PICORB_VM_MRUBYC)

#include "area512_hal.h"
#include "core/terminal/terminal.h"

#include <dirent.h>
#include <stdio.h>
#include <string.h>

typedef struct {
  char directory_path[LINE_MAX];
  char name_prefix[NAME_MAX];
} CompletionTarget;

typedef struct {
  int matching_name_count;
  char common_prefix[NAME_MAX];
  char chosen_name[NAME_MAX];
  int chosen_name_is_directory;
} CompletionMatch;

static int
find_input_word_start_byte_offset(
  const char *input_line,
  int input_byte_count
) {

  int input_byte_offset = input_byte_count;

  while (input_byte_offset > 0 && input_line[input_byte_offset - 1] != ' ')
    input_byte_offset--;

  return input_byte_offset;
}

static void
build_completion_target(
  const char *completion_word,
  CompletionTarget *target
) {

  const char *last_slash_address = strrchr(completion_word, '/');
  int directory_byte_count;

  if (last_slash_address == NULL) {
    target->directory_path[0] = 0;

    strncpy(target->name_prefix, completion_word, NAME_MAX - 1);
    target->name_prefix[NAME_MAX - 1] = 0;

    return;
  }

  directory_byte_count = (int)(last_slash_address - completion_word) + 1;

  if (directory_byte_count > LINE_MAX - 1)
    directory_byte_count = LINE_MAX - 1;

  memcpy(target->directory_path, completion_word, directory_byte_count);
  target->directory_path[directory_byte_count] = 0;

  strncpy(target->name_prefix, last_slash_address + 1, NAME_MAX - 1);
  target->name_prefix[NAME_MAX - 1] = 0;
}

static void
shorten_common_prefix(char *common_prefix, const char *name) {
  int name_byte_offset = 0;

  while (
    common_prefix[name_byte_offset] &&
    common_prefix[name_byte_offset] == name[name_byte_offset]
  )
    name_byte_offset++;

  common_prefix[name_byte_offset] = 0;
}

static void
collect_matching_entry_names(
  Filer *filer,
  const CompletionTarget *target,
  int chosen_match_index,
  CompletionMatch *match
) {

  char expanded_directory_path[LINE_MAX];
  char absolute_directory_path[CURRENT_DIRECTORY_MAX];
  char resolved_path[AREA512_PATH_MAX];
  int target_is_directory;
  DIR *directory_stream;
  struct dirent *directory_entry;

  int name_prefix_byte_count = (int)strlen(target->name_prefix);

  match->matching_name_count = 0;
  match->common_prefix[0] = 0;
  match->chosen_name[0] = 0;
  match->chosen_name_is_directory = 0;

  expand_tilde(
    target->directory_path,
    expanded_directory_path,
    sizeof expanded_directory_path
  );

  if (
    resolve_target_path(
      filer->current_directory,
      expanded_directory_path,
      absolute_directory_path,
      sizeof absolute_directory_path,
      &target_is_directory
    ) != TARGET_RESOLVED
  )
    return;

  if (!target_is_directory)
    return;

  if (
    area512_resolve_data_path(
      absolute_directory_path,
      resolved_path,
      sizeof resolved_path
    ) < 0
  )
    return;

  directory_stream = opendir(resolved_path);

  if (directory_stream == NULL)
    return;

  while ((directory_entry = readdir(directory_stream)) != NULL) {
    if (
      strncmp(
        directory_entry->d_name,
        target->name_prefix,
        name_prefix_byte_count
      )
    )
      continue;

    if (match->matching_name_count == 0) {
      strncpy(match->common_prefix, directory_entry->d_name, NAME_MAX - 1);
      match->common_prefix[NAME_MAX - 1] = 0;
    } else {
      shorten_common_prefix(match->common_prefix, directory_entry->d_name);
    }

    if (match->matching_name_count == chosen_match_index) {
      strncpy(match->chosen_name, directory_entry->d_name, NAME_MAX - 1);
      match->chosen_name[NAME_MAX - 1] = 0;
      match->chosen_name_is_directory =
        (directory_entry->d_type == DT_DIR);
    }

    match->matching_name_count++;
  }

  closedir(directory_stream);
}

static void
collect_matching_terminal_command_names(
  const CompletionTarget *target,
  int chosen_match_index,
  CompletionMatch *match
) {

  int command_index = 0;
  const char *command_name;

  match->matching_name_count = 0;
  match->common_prefix[0] = 0;
  match->chosen_name[0] = 0;
  match->chosen_name_is_directory = 0;

  if (target->directory_path[0])
    return;

  while ((command_name = fetch_command_name(command_index)) != NULL) {
    command_index++;

    if (
      strncmp(
        command_name,
        target->name_prefix,
        strlen(target->name_prefix)
      )
    )
      continue;

    if (match->matching_name_count == 0) {
      strncpy(match->common_prefix, command_name, NAME_MAX - 1);
      match->common_prefix[NAME_MAX - 1] = 0;
    } else {
      shorten_common_prefix(match->common_prefix, command_name);
    }

    if (match->matching_name_count == chosen_match_index) {
      strncpy(match->chosen_name, command_name, NAME_MAX - 1);
      match->chosen_name[NAME_MAX - 1] = 0;
    }

    match->matching_name_count++;
  }
}

static void
replace_terminal_input_word(
  Terminal *terminal,
  int word_start_byte_offset,
  const char *replacement_word
) {

  int replacement_word_byte_count = (int)strlen(replacement_word);

  if (word_start_byte_offset + replacement_word_byte_count > LINE_MAX - 1)
    replacement_word_byte_count = LINE_MAX - 1 - word_start_byte_offset;

  memcpy(
    terminal->input_line + word_start_byte_offset,
    replacement_word,
    replacement_word_byte_count
  );

  terminal->input_byte_count =
    word_start_byte_offset + replacement_word_byte_count;
  terminal->input_line[terminal->input_byte_count] = 0;
}

void
complete_input_word(Filer *filer) {
  Terminal *terminal = filer->terminal;

  int word_start_byte_offset;
  char completion_word[LINE_MAX];
  CompletionTarget completion_target;
  CompletionMatch completion_match;
  char replacement_word[LINE_MAX];

  word_start_byte_offset = find_input_word_start_byte_offset(
    terminal->input_line,
    terminal->input_byte_count
  );

  if (terminal->completion_match_index < 0) {
    strncpy(
      terminal->completion_word,
      terminal->input_line + word_start_byte_offset,
      LINE_MAX - 1
    );

    terminal->completion_word[LINE_MAX - 1] = 0;
    terminal->completion_match_index = 0;
  }

  strncpy(completion_word, terminal->completion_word, LINE_MAX - 1);
  completion_word[LINE_MAX - 1] = 0;

  build_completion_target(completion_word, &completion_target);

  if (word_start_byte_offset == 0)
    collect_matching_terminal_command_names(
      &completion_target,
      terminal->completion_match_index,
      &completion_match
    );
  else
    collect_matching_entry_names(
      filer,
      &completion_target,
      terminal->completion_match_index,
      &completion_match
    );

  if (completion_match.matching_name_count == 0)
    return;

  if (
    completion_match.matching_name_count > 1 &&
    strlen(completion_match.common_prefix) >
      strlen(completion_target.name_prefix)
  ) {

    snprintf(
      replacement_word,
      sizeof replacement_word,
      "%s%s",
      completion_target.directory_path,
      completion_match.common_prefix
    );

    strncpy(terminal->completion_word, replacement_word, LINE_MAX - 1);
    terminal->completion_word[LINE_MAX - 1] = 0;

  } else {
    snprintf(
      replacement_word,
      sizeof replacement_word,
      "%s%s%s",
      completion_target.directory_path,
      completion_match.chosen_name,
      completion_match.chosen_name_is_directory ? "/" : ""
    );

    terminal->completion_match_index =
      (terminal->completion_match_index + 1) %
        completion_match.matching_name_count;
  }

  replace_terminal_input_word(
    terminal,
    word_start_byte_offset,
    replacement_word
  );
}

void
build_autosuggestion(
  Filer *filer,
  char *autosuggestion,
  int autosuggestion_capacity
) {

  Terminal *terminal = filer->terminal;

  int word_start_byte_offset;
  CompletionTarget completion_target;
  CompletionMatch completion_match;
  int name_prefix_byte_count;

  autosuggestion[0] = 0;

  if ((int)strspn(terminal->input_line, " ") == terminal->input_byte_count)
    return;

  word_start_byte_offset =
    find_input_word_start_byte_offset(
      terminal->input_line,
      terminal->input_byte_count
    );

  build_completion_target(
    terminal->input_line + word_start_byte_offset,
    &completion_target
  );

  if (word_start_byte_offset == 0)
    collect_matching_terminal_command_names(
      &completion_target,
      0,
      &completion_match
    );
  else
    collect_matching_entry_names(
      filer,
      &completion_target,
      0,
      &completion_match
    );

  if (completion_match.matching_name_count == 0)
    return;

  name_prefix_byte_count = (int)strlen(completion_target.name_prefix);

  snprintf(
    autosuggestion,
    autosuggestion_capacity,
    "%s%s",
    completion_match.chosen_name + name_prefix_byte_count,
    completion_match.chosen_name_is_directory ? "/" : ""
  );
}

#endif
