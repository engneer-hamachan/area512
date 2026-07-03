#ifndef EDIT_REGISTER_REPEAT_H
#define EDIT_REGISTER_REPEAT_H

#include "core/editor.h"

void remember_last_change(Vim *vim, VimLastChange change);
void repeat_last_change(Vim *vim);

#endif
