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
- `Display.fill_screen(color)`
- `Display.set_brightness(brightness)`
- `Display.show_header_image(path, hold_milliseconds=1000)`

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

## Console

Controls the text console used while a Python file is compiled or executed.

- `Console.reset()`
- `Console.wait_key_if_output()`

## Sleep

Pauses execution for a specified duration.

- `sleep(seconds)`
- `sleep_ms(milliseconds)`
