require 'machine'
require 'io/console'
require 'area512-touch'

class Bomb
  # The window's left edge inside the Filer's frame, so the touch buttons get
  # the room right of the window (Display.move_window).
  TOUCH_WINDOW_LEFT = 12 unless const_defined?(:TOUCH_WINDOW_LEFT)
  TOUCH_BUTTON_LABELS = ["FLAG", "NEW", "QUIT"] unless const_defined?(:TOUCH_BUTTON_LABELS)
  TOUCH_BUTTON_KEYS = [nil, "r", "q"] unless const_defined?(:TOUCH_BUTTON_KEYS)
  FLAG_BUTTON_INDEX = 0 unless const_defined?(:FLAG_BUTTON_INDEX)
  TOUCH_POLL_INTERVAL_MS = 10 unless const_defined?(:TOUCH_POLL_INTERVAL_MS)

  def draw_touch_buttons
    show_touch_button_column(TOUCH_BUTTON_LABELS)
    set_touch_button_on(FLAG_BUTTON_INDEX, @is_flag_mode == true)
  end

  def press_touch_button(button_index)
    key = TOUCH_BUTTON_KEYS[button_index]
    return key if key
    @is_flag_mode = !@is_flag_mode
    set_touch_button_on(FLAG_BUTTON_INDEX, @is_flag_mode)
    nil
  end

  def apply_tap(x, y)
    cell_x = (x - OX) / CW
    cell_y = (y - OY) / CH
    return nil unless in_bounds?(cell_x, cell_y)
    @cx = cell_x
    @cy = cell_y
    @is_flag_mode ? "f" : " "
  end

  def read_pending_key
    key = nil
    STDIN.raw do
      key = STDIN.read_nonblock(1)
    end
    key
  end

  def read_char
    unless @is_touch_button_drawn
      draw_touch_buttons
      @is_touch_button_drawn = true
    end
    while true
      key = read_pending_key
      return key if key
      event = read_touch_event
      while event
        return "" if @quit
        key = apply_tap(event[1], event[2]) if event[0] == "TAP"
        key = press_touch_button(event[1]) if event[0] == "BUTTON"
        return key if key
        event = read_touch_event
      end
      sleep_ms TOUCH_POLL_INTERVAL_MS
    end
  end
end
