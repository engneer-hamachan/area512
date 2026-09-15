#ifndef AREA512_MARKDOWN_H
#define AREA512_MARKDOWN_H

#include <mrubyc.h>

void define_markdown_class(mrbc_vm *virtual_machine);

#ifdef AREA512_EXT_DISPLAY
int show_internal_markdown(const char *path);
void show_next_internal_markdown_page(void);
void show_previous_internal_markdown_page(void);
void hide_internal_markdown(void);
#endif

#endif
