# Area512Widget

Themed UI components for Area512/Cardputer apps. One `require` gives every app
the same amber palette, screen-size-aware layout, and font metrics — no more
copying `COL_*` constants and pixel offsets between apps.

```ruby
require 'area512-sprite'
require 'area512-widget'
```

All drawing goes onto the app's own full-screen `Sprite` (font size 12), passed
as the first argument. Widgets never call `push`; the app keeps its usual
`draw everything, then push(0, 0)` frame loop. Only the modal helpers
(`input`, `menu`, ...) run their own key loop and push by themselves.

```ruby
def draw
  @sp.fill(Widget.bg)
  Widget.header(@sp, "SCHEDULER", "JUL 2026")
  @list.draw(@sp)
  Widget.footer(@sp, @msg)
  @sp.push(0, 0)
end

def handle(key)
  return if @list.handle(key)
  case key
  when "a"
    title = Widget.input(@sp, "TITLE:")
    add_event(title) if title
  when "q"
    @quit = true
  end
end

# main loop:  draw; handle(Widget.read_key) while !@quit
```

## Theme and metrics

| Method | Value | Meaning |
|---|---|---|
| `Widget.bg` | `0x000000` | background |
| `Widget.amber` | `0xF5972D` | frames, bands, accents |
| `Widget.dim` | `0xCFA45F` | normal text |
| `Widget.gold` | `0xFFD966` | selection, titles |
| `Widget.dark` | `0x241604` | selected-row / panel fill |
| `Widget.char_width` | 6 | ASCII glyph width (font 12) |
| `Widget.row_height` | 16 | standard list/table row |
| `Widget.header_height` | 17 | header band height |
| `Widget.body_top` / `body_bottom` / `body_height` | 20 / 119 / 100 | content area between header and footer (values for 240x135) |

Text measurement uses the real renderer, so Japanese text measures correctly:

```ruby
Widget.text_width(sp, text)      # => px
Widget.clip(sp, text, width_px)  # => String, ">"-terminated when cut
```

## Screen chrome

```ruby
Widget.header(sp, title, right = "")  # top band; gold title, dim right-aligned text
Widget.footer(sp, message)            # bottom rule + message (clipped)
Widget.hints(sp, [["ENT", "open"], ["q", "quit"]])  # footer-position key chips
Widget.separator(sp, y)               # full-width horizontal rule
Widget.vseparator(sp, x, y, h)        # vertical rule
Widget.tabs(sp, y, ["CAL", "DAY"], active_index)
Widget.battery(sp, x, y)              # battery icon + charge level
Widget.splash(sp, title, subtitle = "")  # full-screen title in the 24px font
```

## Containers and text

```ruby
Widget.panel(sp, x, y, w, h)                # dark fill + amber frame
Widget.titled_panel(sp, x, y, w, h, title)
Widget.toast(sp, message)                   # bottom-center chip; app decides how long to show it
Widget.text_center(sp, y, text, color = Widget.dim)
Widget.text_right(sp, right_x, y, text, color = Widget.dim)
Widget.center_lines(sp, [["Cleared!", Widget.gold], [""], ["Press any key."]])
Widget.wrap_text(sp, x, y, w, h, text, color = Widget.dim)  # => lines drawn
Widget.marquee(sp, x, y, w, text, offset)   # => text px width; advance offset per frame
Widget.big_text(sp, x, y, text, color = Widget.gold)        # 24px font
```

## Tables and records

```ruby
Widget.cell(sp, x, y, w, h, text, selected)
Widget.table_header(sp, x, y, [60, 90, 60], ["ID", "NAME", "QTY"])
Widget.table_row(sp, x, y, [60, 90, 60], ["3", "Bolt M3", "12"], selected)
Widget.field(sp, x, y, w, label, value, focused)  # settings row: dim label, gold value
```

## Indicators and charts

```ruby
Widget.gauge(sp, x, y, w, value, max)             # read-only progress bar
Widget.slider(sp, x, y, w, value, max, focused)   # track + knob
Widget.scrollbar(sp, x, y, h, top, visible, total)    # hidden when total <= visible
Widget.hscrollbar(sp, x, y, w, left, visible, total)
Widget.badge(sp, x, y, text)                      # inverted chip
Widget.busy(sp, x, y, frame)                      # 4-phase busy marker; pass a frame counter
Widget.page_dots(sp, y, count, active)            # centered page indicator
Widget.bar_chart(sp, x, y, w, h, values, max)     # max <= 0 auto-scales to the data
Widget.line_chart(sp, x, y, w, h, values, max)
```

## Form controls

```ruby
Widget.button(sp, x, y, w, label, focused)
Widget.checkbox(sp, x, y, label, checked, focused)
Widget.radio(sp, x, y, label, selected, focused)
Widget.toggle(sp, x, y, on, focused)
Widget.spinner(sp, x, y, w, text, focused)   # "< text >"; the app handles left/right keys
```

Widgets do not manage focus. The app keeps a focus index and passes `focused`
to each control; `storage/home/tool/gallery` shows the pattern.

## Modals

Each runs its own key loop over the current sprite content and returns the
result. The app simply redraws its next frame afterwards.

```ruby
Widget.input(sp, label, initial = "")   # => String on Enter (may be ""), nil on ESC
Widget.input_number(sp, label, initial, min, max)  # => Integer clamped, nil on ESC
Widget.confirm(sp, question)            # => true (y) / false (n, ESC)
Widget.dialog(sp, message, ["OK", "Cancel"])  # => button index, nil on ESC
Widget.menu(sp, title, ["Open", "Save"], initial = 0)  # => item index, nil on ESC
Widget.alert(sp, message)               # any key; => nil
```

## Key input

```ruby
Widget.read_key  # => "UP" "DOWN" "LEFT" "RIGHT" "ENTER" "ESC" "BS" or a 1-char String
```

Escape sequences are decoded; keypad arrows that arrive as bare `; . / ,` are
returned as-is so they stay typable — list widgets accept them in `handle`.

## WidgetList

Scrollable, selectable rows with an optional tag column and multi-select marks.

```ruby
list = WidgetList.new
list.area(x, y, w, h)          # default: whole body area
list.clear
list.add("Meeting", "07:30")   # tag is optional, drawn amber on the left
list.empty_text = "No schedule"
list.show_marks = true
list.toggle_mark               # cursor row
list.mark(i, true) / list.marked?(i)
list.count / list.index / list.index = n / list.top
list.handle(key)               # consumes UP/DOWN, k/j, ;/. — returns true when consumed
list.draw(sp)
```

Capacity: 128 rows, 39-byte text, 11-byte tag (longer input is truncated).

## WidgetTextView

Word-wrapped scrollable text (help screens, long descriptions).

```ruby
view = WidgetTextView.new
view.area(x, y, w, h)     # default: whole body area
view.text = long_string   # copied, up to 2048 bytes
view.scroll / view.scroll = n
view.handle(key)          # UP/DOWN, k/j, ;/.
view.draw(sp)
```

## Demo

`storage/home/tool/gallery` shows every component across six tabs
(PARTS / TABLE / CHART / LIST / TEXT / MODAL) and doubles as the on-device
test. Type signatures for the ti checker live in
`template/app/.ti-config/area512widgets.json` (plus `widget_list.json` and
`widget_text_view.json`).
