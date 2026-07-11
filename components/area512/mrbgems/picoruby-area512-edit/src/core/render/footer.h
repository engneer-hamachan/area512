#ifndef EDIT_RENDER_FOOTER_H
#define EDIT_RENDER_FOOTER_H

#include "core/editor.h"

void draw_vim_footer(void *vim_context, VimCanvas *canvas);
void draw_vim_cursor(void *vim_context, VimCanvas *canvas);

void show_message(Vim *vim, const char *text, int byte_length);
void show_message_c_string(Vim *vim, const char *text);
void clear_command(Vim *vim);

#endif
