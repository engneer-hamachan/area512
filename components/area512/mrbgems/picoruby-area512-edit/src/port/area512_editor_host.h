#ifndef AREA512_EDITOR_HOST_H
#define AREA512_EDITOR_HOST_H

void compute_editor_grid(int *columns, int *rows);

// sequence must hold 2 bytes; returns how many following bytes were read (0-2).
int read_escape_sequence(char *sequence);

#endif
