#if defined(PICORB_VM_MRUBYC)

#include "area512_hal.h"
#include "core/widget.h"

static int
read_csi_final_byte(void) {
  int byte;

  do {
    byte = area512_console_getch_timeout(40);
  } while (byte >= ' ' && byte < '@');

  return byte;
}

static WidgetKey
named_key(WidgetKeyKind kind) {
  WidgetKey key = {kind, 0};
  return key;
}

static WidgetKey
byte_key(int byte) {
  WidgetKey key = {WIDGET_KEY_BYTE, (char)byte};
  return key;
}

WidgetKey
area512_widget_read_key(void) {
  int first_byte = area512_console_getch_block();

  if (first_byte == 27) {
    if (area512_console_getch_timeout(40) != '[')
      return named_key(WIDGET_KEY_ESCAPE);

    switch (read_csi_final_byte()) {
    case 'A':
      return named_key(WIDGET_KEY_UP);
    case 'B':
      return named_key(WIDGET_KEY_DOWN);
    case 'C':
      return named_key(WIDGET_KEY_RIGHT);
    case 'D':
      return named_key(WIDGET_KEY_LEFT);
    default:
      return area512_widget_read_key();
    }
  }

  if (first_byte == '\r' || first_byte == '\n')
    return named_key(WIDGET_KEY_ENTER);

  if (first_byte == '\b' || first_byte == 127)
    return named_key(WIDGET_KEY_BACKSPACE);

  return byte_key(first_byte);
}

WidgetKey
area512_widget_read_text_key(void) {
  for (;;) {
    WidgetKey key = area512_widget_read_key();

    if (key.kind != WIDGET_KEY_UP && key.kind != WIDGET_KEY_DOWN &&
        key.kind != WIDGET_KEY_LEFT && key.kind != WIDGET_KEY_RIGHT)
      return key;
  }
}

const char *
area512_widget_key_name(WidgetKey key, char byte_string[2]) {
  static const char *const names[] =
    {NULL, "UP", "DOWN", "LEFT", "RIGHT", "ENTER", "ESC", "BS"};

  if (key.kind != WIDGET_KEY_BYTE)
    return names[key.kind];

  byte_string[0] = key.byte;
  byte_string[1] = 0;
  return byte_string;
}

#endif
