# PicoRuby Features

AREA512 provides the APIs on this page as built-ins. Each app starts with only
`SD`, `File`, `Dir`, `Area512`, and `Sandbox` loaded; `require` the others
before use.

| API | `require` |
| --- | --- |
| `Display`, `Sprite` | `require 'area512-sprite'` |
| `Dot` | `require 'area512-dot'` |
| `Widget`, `WidgetList`, `WidgetTextView` | `require 'area512-widget'` |
| `Touch` (ES3C28P only) | `require 'area512-touch'` |
| `GPIO` | `require 'gpio'` |
| `I2C` | `require 'i2c'` |
| `IO` | `require 'io/console'` |
| `RNG` | `require 'rng'` |
| `SD`, `File`, `Dir`, `Area512`, `Sandbox` | Not needed |

## Reference Implementation

The preinstalled [Widget Gallery](storage/home/tool/gallery) application shows
PicoRuby implementations using `Display`, `Sprite`, `Widget`, `WidgetList`, and
`WidgetTextView`.

## Display

Controls the device display.

- `Display.width`
- `Display.height`
- `Display.screen_width`, `Display.screen_height`
- `Display.full_screen`
- `Display.fill_screen(color)`
- `Display.brightness = brightness`
- `Display.console`
- `Display.subwindow(x, y, width, height)` places a subwindow outside the
  window, in window coordinates, and `Display.push_subwindow(sprite)` draws a
  `Sprite` or `QuaternarySprite` there. Taps on it come from `Touch.read` with
  window coordinates outside the window. Available only in the ES3C28P
  firmware.
- `Display.move_window(left, top)` moves the window to (`left`, `top`) in
  screen pixels, `top` being its position while the software keyboard is
  closed. The window's border is drawn there, and the app draws its inside
  again. `Display.read_window_top_without_soft_keyboard` returns that
  `top`. Available only in the ES3C28P firmware.

`width` and `height` report the area the app draws on. On the 320x240 display
builds the filer runs each app in a 240x135 window, so they report 240x135
there too. `screen_width` and `screen_height` always report the whole display.
`full_screen` lets the app draw on the whole display until it returns; it does
nothing when there is no window. Call it before creating screen-sized sprites:

```ruby
Display.full_screen if Display.screen_width > Display.width
```

## Sprite

Creates an off-screen drawing surface that can be pushed to the display.

- `Sprite.new(width, height, font_size = nil)`
- `width`, `height`, `delete`
- `fill(color)`, `pixel(x, y, color)`
- `line(x0, y0, x1, y1, color)`
- `rect(x, y, width, height, color)`
- `fill_rect(x, y, width, height, color)`
- `circle(x, y, radius, color)`
- `fill_circle(x, y, radius, color)`
- `text(x, y, string, color)`
- `push(x, y, transparent_color = nil)`

## Dot

Loads and edits `.a5d` dot images.

- `Dot.load(path)`, `Dot.edit(path)`
- `width`, `height`
- `painted_left`, `painted_top`, `painted_right`, `painted_bottom`
- `push(sprite, x, y)`
- `Dot.overlap?(first, first_x, first_y, second, second_x, second_y,
  use_hitbox = false)`

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

- `WidgetList.new`
- `area`, `clear`, `add`
- `empty_text=`, `show_marks=`, `toggle_mark`, `mark`, `marked?`
- `count`, `index`, `index=`, `top`, `handle`, `draw`

## WidgetTextView

Manages and draws scrollable text.

- `WidgetTextView.new`
- `area`, `text=`, `scroll`, `scroll=`, `handle`, `draw`

## Touch

Reads the touch panel and shows the software keyboard. Available only in the
ES3C28P firmware.

- `Touch.read` returns `nil` or `[kind, x, y]`. `kind` is `"TAP"`, `"DRAG"` or
  `"RELEASE"`, and `x`, `y` are window coordinates. A finger lifted without
  moving gives one `"TAP"` at the point it touched; a moving finger gives
  `"DRAG"` events and then one `"RELEASE"` where it was lifted.
- `Touch.open_keyboard`, `Touch.close_keyboard` show and hide the software
  keyboard under the window. The window moves up while the keyboard is shown,
  so redraw the screen after each call. Keys typed on it arrive through `STDIN`
  like keyboard keys.
- `Touch.bluetooth_keyboard_connected?`

```ruby
require 'area512-touch'

while true
  key = STDIN.read_nonblock(1)
  event = Touch.read
  sleep_ms 10 if key.nil? && event.nil?
end
```

## GPIO

Reads and writes GPIO pins and configures their direction, pull mode, open-drain
mode, and alternate function.

- `GPIO.new(pin, flags, alt_function = nil)`
- `read`, `write`, `high?`, `low?`
- `setmode`, `set_function`, `set_dir`, `set_pull`, `open_drain`
- `GPIO.read_at`, `GPIO.write_at`, `GPIO.high_at?`, `GPIO.low_at?`
- `GPIO.set_function_at`, `GPIO.set_dir_at`, `GPIO.pull_up_at`,
  `GPIO.pull_down_at`, `GPIO.open_drain_at`

## I2C

Communicates with I2C devices.

- `I2C.new(unit:, frequency:, sda_pin:, scl_pin:, timeout:)`
- `write`, `read`, `scan`

## SD, File, and Dir

Mounts the microSD card and accesses its files and directories.

- `SD.mount`, `SD.unmount`, `SD.exist?`, `SD.read`, `SD.write`
- `File.new`, `File.open`, `read`, `close`
- `File.exist?`, `File.file?`, `File.directory?`, `File.unlink`, `File.rename`
- `Dir.open`, `read`, `close`
- `Dir.exist?`, `Dir.mkdir`, `Dir.rmdir`

## IO

Reads keys and controls the console input mode.

- `IO.open`, `IO.get_cursor_position`, `IO.wait_terminal`, `IO.clear_screen`
- `getch`, `read_nonblock`
- `raw`, `raw!`, `cooked`, `cooked!`, `echo?`, `echo=`

## RNG

Provides random values.

- `RNG.random_int`
- `RNG.random_string(length)`
- `RNG.uuid`

## Area512

Hands off to another app. Available without `require`.

- `Area512.run_next(path)`

When the current app returns, the file manager runs `path` right away, as if
it were selected and run with `R`: an app directory such as `/home/game/bomb`,
or a `.rb`, `.mrb`, `.py` or `.mpy` file. The last call wins and an empty path
cancels. The path is used once, and an app that ends with an error does not
hand off, so a failing app cannot start a loop.

```ruby
Area512.run_next("/home/game/bomb")
```

## Sandbox

Compiles and executes Ruby source or `.mrb` bytecode in a sandbox task.

- `Sandbox.new`
- `compile`, `compile_from_memory`, `execute`
- `exec_mrb`, `exec_mrb_from_memory`, `load_file`, `load_manifest`
- `wait`, `state`, `result`, `error`
- `resume`, `suspend`, `stop`, `terminate`
- `free_parser`, `cleanup`
