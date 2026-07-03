#if defined(PICORB_VM_MRUBYC)

#include "core/filer.h"
#include "area512_hal.h"

int
area512_filer_read_key(void) {
  int first_byte = area512_console_getch_block();

  if (first_byte == 27) {
    if (area512_console_getch_timeout(40) != '[')
      return first_byte;
    switch (area512_console_getch_timeout(40)) {
    case 'A':
      return KEY_UP;
    case 'B':
      return KEY_DOWN;
    case 'C':
      return KEY_RIGHT;
    case 'D':
      return KEY_LEFT;
    default:
      return first_byte;
    }
  }

  switch (first_byte) {
  // keypad arrows arrive as bare ASCII ; . / ,
  case ';':
    return KEY_UP;
  case '.':
    return KEY_DOWN;
  case '/':
    return KEY_RIGHT;
  case ',':
    return KEY_LEFT;
  case 'k':
    return KEY_UP;
  case 'j':
    return KEY_DOWN;
  case 'u':
  case '\b':
  case 127:
    return KEY_LEFT;
  case '\r':
  case '\n':
    return KEY_RIGHT;
  case 'q':
    return KEY_QUIT;
  case 'c':
    return KEY_COMPILE;
  case 'a':
    return KEY_COMPILE_ALL;
  case 'e':
    return KEY_EDIT;
  case 'R':
    return KEY_RUN_DIR;
  case 'x':
    return KEY_DELETE;
  case 'N':
    return KEY_NEW_FILE;
  case 'K':
    return KEY_NEW_DIR;
  case 'r':
    return KEY_REBOOT;
  default:
    return first_byte;
  }
}

#endif
