require 'machine'
require 'io/console'
require 'area512-touch'

class PaintApp
  # The window's left edge inside the Filer's frame, so the touch buttons get
  # the room right of the window (Display.move_window).
  TOUCH_WINDOW_LEFT = 12 unless const_defined?(:TOUCH_WINDOW_LEFT)
  TOUCH_BUTTON_LABELS = ["-", "+", "ERASE", "CLEAR", "QUIT"] unless const_defined?(:TOUCH_BUTTON_LABELS)
  TOUCH_BUTTON_KEYS = ["-", "+", nil, "x", "q"] unless const_defined?(:TOUCH_BUTTON_KEYS)
  ERASE_BUTTON_INDEX = 2 unless const_defined?(:ERASE_BUTTON_INDEX)
  TOUCH_POLL_INTERVAL_MS = 10 unless const_defined?(:TOUCH_POLL_INTERVAL_MS)
  PALETTE_SWATCH_LEFT = 150 unless const_defined?(:PALETTE_SWATCH_LEFT)
  PALETTE_SWATCH_SPACING = 8 unless const_defined?(:PALETTE_SWATCH_SPACING)

  def draw_touch_buttons
    show_touch_button_column(TOUCH_BUTTON_LABELS)
    set_touch_button_on(ERASE_BUTTON_INDEX, @is_eraser_on == true)
  end

  def press_touch_button(button_index)
    key = TOUCH_BUTTON_KEYS[button_index]
    return key if key
    @is_eraser_on = !@is_eraser_on
    set_touch_button_on(ERASE_BUTTON_INDEX, @is_eraser_on)
    nil
  end

  def apply_tap(x, y)
    return nil if y < 0 || y >= STATUS_H || x < PALETTE_SWATCH_LEFT
    swatch_index = (x - PALETTE_SWATCH_LEFT) / PALETTE_SWATCH_SPACING
    return nil if swatch_index >= PALETTE.length
    @color_index = swatch_index - 1
    "c"
  end

  def paint_at_touch_point(event)
    @cursor_x = event[1] / CELL
    @cursor_y = (event[2] - CANVAS_Y) / CELL
    if !@is_touch_stroking || @last_paint_x < 0
      @last_paint_x = @cursor_x
      @last_paint_y = @cursor_y
    end
    @is_touch_stroking = event[0] == "DRAG"
    mark = @is_eraser_on ? PALETTE_MARK[ERASER_INDEX] : @color_mark
    draw_line(@last_paint_x, @last_paint_y, @cursor_x, @cursor_y, mark)
    @last_paint_x = @cursor_x
    @last_paint_y = @cursor_y
    @is_eraser_on ? "e" : " "
  end

  def read_pending_key
    key = nil
    STDIN.raw do
      key = STDIN.read_nonblock(1)
    end
    key
  end

  def read_key
    unless @is_touch_button_drawn
      draw_touch_buttons
      @is_touch_button_drawn = true
    end
    while true
      key = read_pending_key
      return key if key
      stroke_key = nil
      event = read_touch_event
      while event
        return "" if @quit
        x = event[1]
        y = event[2]
        if event[0] == "BUTTON"
          @is_touch_stroking = false
          key = press_touch_button(x)
          return key if key
        elsif event[0] != "RELEASE" && x >= 0 && x < APP_W && y >= CANVAS_Y && y < APP_H
          stroke_key = paint_at_touch_point(event)
        else
          @is_touch_stroking = false
          key = event[0] == "TAP" ? apply_tap(x, y) : nil
          return key if key
        end
        event = read_touch_event
      end
      return stroke_key if stroke_key
      sleep_ms TOUCH_POLL_INTERVAL_MS
    end
  end
end
