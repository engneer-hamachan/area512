#include "core/mode/command.h"
#include "core/register/search.h"
#include "core/render/footer.h"
#include <stddef.h>
#include <string.h>

static int
is_line_number_command(const char *command, int command_byte_length) {
  if (command_byte_length <= 1 || command[0] != ':')
    return 0;

  for (int i = 1; i < command_byte_length; i++)
    if (!(command[i] >= '0' && command[i] <= '9'))
      return 0;

  return 1;
}

static int
command_matches_literal(
  const char *command,
  int command_byte_length,
  const char *literal
) {

  int literal_byte_length = (int)strlen(literal);

  return command_byte_length == literal_byte_length &&
         memcmp(command, literal, (size_t)literal_byte_length) == 0;
}

VimStatus
execute_command(Vim *vim) {
  const char *command = vim->status.command.bytes;
  int command_byte_length = vim->status.command.byte_length;

  int name_start = 0;
  while (name_start < command_byte_length && command[name_start] == ' ')
    name_start++;

  int name_end = name_start;
  while (name_end < command_byte_length && command[name_end] != ' ')
    name_end++;

  int argument_start = name_end;
  while (argument_start < command_byte_length && command[argument_start] == ' ')
    argument_start++;

  int argument_end = command_byte_length;
  while (argument_end > argument_start && command[argument_end - 1] == ' ')
    argument_end--;

  const char *name = command + name_start;
  int name_byte_length = name_end - name_start;
  const char *argument = command + argument_start;
  int argument_byte_length = argument_end - argument_start;

  if (name_byte_length == 0)
    return VIM_CONTINUE;

  if (argument_byte_length > 0) {
    if (command_matches_literal(name, name_byte_length, ":w")) {
      vim_set_filepath(vim, argument, argument_byte_length);

      return VIM_SAVE;
    }

    show_message_c_string(vim, "Not an editor command");

    return VIM_CONTINUE;
  }

  if (is_line_number_command(name, name_byte_length)) {
    int line_number = 0;

    for (int i = 1; i < name_byte_length; i++)
      line_number = line_number * 10 + (name[i] - '0');

    vim_buffer_go_to_line(BUFFER, line_number - 1, 1);

    return VIM_CONTINUE;
  }

  if (command_matches_literal(name, name_byte_length, ":q")) {
    if (BUFFER->changed) {
      show_message_c_string(vim, "No write since last change");
      return VIM_CONTINUE;
    }

    return VIM_QUIT;
  }

  if (command_matches_literal(name, name_byte_length, ":q!"))
    return VIM_QUIT;

  if (command_matches_literal(name, name_byte_length, ":w"))
    return VIM_SAVE;

  if (command_matches_literal(name, name_byte_length, ":wq") ||
      command_matches_literal(name, name_byte_length, ":x"))

    return VIM_SAVE_QUIT;

  show_message_c_string(vim, "Not an editor command");

  return VIM_CONTINUE;
}

VimStatus
handle_command_mode(Vim *vim, int key) {
  switch (key) {
  case 10:   // '\n' (LF), Enter
  case 13: { // '\r' (CR), Enter
    VimStatus status = VIM_CONTINUE;

    if (vim->input.mode == VIM_MODE_SEARCH) {
      const char *pattern = "";
      int pattern_byte_length = 0;

      if (vim->status.command.byte_length > 1) {
        pattern = vim->status.command.bytes + 1;
        pattern_byte_length = vim->status.command.byte_length - 1;
      }

      if (pattern_byte_length > 0)
        vim_string_set(&vim->search.last_search, pattern, pattern_byte_length);

      search_pattern(vim, pattern, pattern_byte_length, 1, 1);

    } else {
      status = execute_command(vim);
    }

    clear_command(vim);

    vim->input.mode = VIM_MODE_NORMAL;
    REDRAW(VIM_REDRAW_ALL);

    return status;
  }

  case 8:   // '\b' (BS), Backspace
  case 127: // DEL, Backspace
    if (vim->status.command.byte_length > 0)
      vim->status.command.byte_length -= 1;

    if (vim->status.command.byte_length == 0)
      vim->input.mode = VIM_MODE_NORMAL;

    REDRAW(VIM_REDRAW_FOOTER);

    break;

  default:
    if (key >= 32 && key <= 126) { // printable ASCII, ' ' (space) to '~'
      vim_string_append_byte(&vim->status.command, (char)key);
      REDRAW(VIM_REDRAW_FOOTER);
    }

    break;
  }

  return VIM_CONTINUE;
}
