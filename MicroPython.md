# MicroPython Features

AREA512 provides the APIs on this page as built-ins. They can be used without
`import`.

## Reference Implementation

The preinstalled [Space Lander](storage/home/game/space_lander) application
shows a MicroPython implementation using `Display`, `Sprite`, `IO`, and
`sleep_ms`.

## Display

Controls the device display.

- `Display.width()`
- `Display.height()`
- `Display.screen_width()`, `Display.screen_height()`
- `Display.full_screen()`
- `Display.fill_screen(color)`
- `Display.set_brightness(brightness)`
- `Display.show_header_image(path, hold_milliseconds=1000)`
- `Display.subwindow(x, y, width, height)` places a subwindow outside the
  window, in window coordinates, and `Display.push_subwindow(sprite)` draws a
  `Sprite` or `QuaternarySprite` there. Taps on it come from `Touch.read()`
  with window coordinates outside the window. Available only in the ES3C28P
  firmware.
- `Display.move_window(left, top)` moves the window to (`left`, `top`) in
  screen pixels, `top` being its position while the software keyboard is
  closed. The window's border is drawn there, and the app draws its inside
  again. `Display.read_window_top_without_soft_keyboard()` returns that
  `top`. Available only in the ES3C28P firmware.

`width()` and `height()` report the area the app draws on. On the 320x240
display builds the filer runs each app in a 240x135 window, so they report
240x135 there too. `screen_width()` and `screen_height()` always report the
whole display. `full_screen()` lets the app draw on the whole display until it
returns; it does nothing when there is no window. Call it before creating
screen-sized sprites:

```python
if Display.screen_width() > Display.width():
    Display.full_screen()
```

## Sprite

Creates an off-screen drawing surface that can be pushed to the display.

- `Sprite(width, height, font_size=None)`
- `width()`, `height()`, `delete()`
- `fill(color)`, `pixel(x, y, color)`
- `line(x0, y0, x1, y1, color)`
- `rect(x, y, width, height, color)`
- `fill_rect(x, y, width, height, color)`
- `circle(x, y, radius, color)`
- `fill_circle(x, y, radius, color)`
- `text(x, y, string, color)`
- `push(x, y, transparent=None)`

## Dot

Loads and edits `.a5d` dot images.

- `Dot.load(path)`, `Dot.edit(path)`
- `width()`, `height()`
- `painted_left()`, `painted_top()`, `painted_right()`, `painted_bottom()`
- `push(sprite, x, y)`
- `Dot.overlap(first, first_x, first_y, second, second_x, second_y,
  use_hitbox=False)`

## Widget

Draws AREA512 user-interface components on a `Sprite` and reads keys.

- Colors and layout: `bg`, `amber`, `dim`, `gold`, `dark`, `char_width`,
  `row_height`, `header_height`, `body_top`, `body_bottom`, `body_height`
- Text and input: `text_width`, `clip`, `read_key`, `text_center`, `text_right`,
  `center_lines`, `wrap_text`, `marquee`, `big_text`
- Layout: `header`, `footer`, `hints`, `separator`, `vseparator`, `tabs`,
  `battery`, `splash`, `panel`, `titled_panel`, `toast`
- Data display: `cell`, `table_header`, `table_row`, `field`, `gauge`, `slider`,
  `scrollbar`, `hscrollbar`, `badge`, `busy`, `page_dots`, `bar_chart`,
  `line_chart`
- Controls: `button`, `checkbox`, `radio`, `toggle`, `spinner`, `input`,
  `input_number`, `confirm`, `dialog`, `menu`, `alert`

## WidgetList

Manages and draws a selectable, scrollable list.

- `WidgetList()`
- `area()`, `clear()`, `add()`
- `set_empty_text()`, `set_show_marks()`, `toggle_mark()`, `mark()`, `marked()`
- `count()`, `index()`, `set_index()`, `top()`, `handle()`, `draw()`

## WidgetTextView

Manages and draws scrollable text.

- `WidgetTextView()`
- `area()`, `set_text()`, `scroll()`, `set_scroll()`, `handle()`, `draw()`

## GPIO

Reads and writes GPIO pins and configures their direction, pull mode, open-drain
mode, and alternate function.

- `GPIO(pin, flags, alt_function=0)`
- `pin()`, `read()`, `write()`, `high()`, `low()`
- `setmode()`, `set_function()`, `set_dir()`, `set_pull()`, `open_drain()`
- `GPIO.read_at()`, `GPIO.write_at()`, `GPIO.high_at()`, `GPIO.low_at()`
- `GPIO.set_function_at()`, `GPIO.set_dir_at()`, `GPIO.pull_up_at()`,
  `GPIO.pull_down_at()`, `GPIO.open_drain_at()`

## ADC

Reads an ADC pin.

- `ADC(pin)`
- `input()`
- `read()`, `read_voltage()`
- `read_raw()`

## SD, File, and Dir

Mounts the microSD card and accesses its files and directories.

- `SD.mount()`, `SD.unmount()`, `SD.exist()`, `SD.mkdir()`
- `SD.read()`, `SD.write()`, `SD.restore_seed()`
- `File(path, mode=None)`, `read()`, `close()`
- `File.exist()`, `File.file()`, `File.directory()`, `File.unlink()`,
  `File.rename()`
- `Dir(path)`, `read()`, `close()`
- `Dir.exist()`, `Dir.mkdir()`, `Dir.rmdir()`

## IO

Reads keys and controls the console input mode.

- `IO()`
- `getch()`, `read_nonblock()`
- `raw()`, `cooked()`, `echo()`, `set_echo()`

## RNG

Provides random values.

- `RNG.random_int()`
- `RNG.random_string(length)`
- `RNG.uuid()`

## Area512

Hands off to another app.

- `Area512.run_next(path)`

When the current app returns, the file manager runs `path` right away, as if
it were selected and run with `R`: an app directory such as `/home/game/bomb`,
or a `.py`, `.mpy`, `.rb` or `.mrb` file. The last call wins and an empty path
cancels. The path is used once, and an app that ends with an error does not
hand off, so a failing app cannot start a loop.

```python
Area512.run_next("/home/game/bomb")
```

## Console

Controls the text console used while a Python file is compiled or executed.

- `Console.reset()`
- `Console.wait_key_if_output()`

## Touch

Reads the touch panel and shows the software keyboard. Available only in the
ES3C28P firmware.

- `Touch.read()` returns `None` or `(kind, x, y)`. `kind` is `"TAP"`, `"DRAG"`
  or `"RELEASE"`, and `x`, `y` are window coordinates. A finger lifted without
  moving gives one `"TAP"` at the point it touched; a moving finger gives
  `"DRAG"` events and then one `"RELEASE"` where it was lifted.
- `Touch.open_keyboard()`, `Touch.close_keyboard()` show and hide the software
  keyboard under the window. The window moves up while the keyboard is shown,
  so redraw the screen after each call. Keys typed on it arrive through `IO`
  like keyboard keys.
- `Touch.bluetooth_keyboard_connected()`

```python
while True:
    key = IO().read_nonblock(1)
    event = Touch.read()
    if key is None and event is None:
        sleep_ms(10)
```

## Sleep

Pauses execution for a specified duration.

- `sleep(seconds)`
- `sleep_ms(milliseconds)`
