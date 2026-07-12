#include "port/area512_editor_host.h"

#include "port/area512_editor_canvas.h"

#include "area512_hal.h"
#include "io-console.h"
#include <stdbool.h>

void
compute_editor_grid(int *columns, int *rows) {
  int grid_columns = area512_gfx_width() / EDIT_CHAR_WIDTH;
  int grid_rows = area512_gfx_height() / EDIT_ROW_HEIGHT;

  *columns = grid_columns < 1 ? 1 : grid_columns;
  *rows = grid_rows < 1 ? 1 : grid_rows;
}

int
read_escape_sequence(char *sequence) {
  int first_following = area512_console_getch_timeout(20);

  if (first_following < 0)
    return 0;

  sequence[0] = (char)first_following;

  int second_following = area512_console_getch_timeout(20);

  if (second_following < 0)
    return 1;

  sequence[1] = (char)second_following;

  return 2;
}
