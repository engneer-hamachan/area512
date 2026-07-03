#ifndef EDIT_MODE_OPERATOR_H
#define EDIT_MODE_OPERATOR_H

#include "core/editor.h"

void handle_operator(Vim *vim, int key);
int apply_operator_motion(Vim *vim, VimOperator operator_kind, int key);

#endif
