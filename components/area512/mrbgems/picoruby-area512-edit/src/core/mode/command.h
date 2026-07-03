#ifndef EDIT_MODE_COMMAND_H
#define EDIT_MODE_COMMAND_H

#include "core/editor.h"

VimStatus handle_command_mode(Vim *vim, int key);
VimStatus execute_command(Vim *vim);

#endif
