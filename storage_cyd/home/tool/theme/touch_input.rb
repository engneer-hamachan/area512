require 'area512-touch'

class ThemeEditor
  # The window's left edge inside the Filer's frame, so the touch buttons get
  # the room right of the window (Display.move_window).
  TOUCH_WINDOW_LEFT = 12 unless const_defined?(:TOUCH_WINDOW_LEFT)
  CHANNEL_LABELS = ["R", "G", "B"] unless const_defined?(:CHANNEL_LABELS)
  # The first edit button shows the channel and moves to the next one.
  EDIT_TOUCH_BUTTON_KEYS = ["l", "[", "-", "+", "]", nil] unless const_defined?(:EDIT_TOUCH_BUTTON_KEYS)
  EDIT_TOUCH_BUTTON_LABELS = ["", "-16", "-1", "+1", "+16", "MORE"] unless const_defined?(:EDIT_TOUCH_BUTTON_LABELS)
  # The last menu button goes back to the edit buttons without a key.
  MENU_TOUCH_BUTTON_KEYS = ["\t", "s", "ESC", nil] unless const_defined?(:MENU_TOUCH_BUTTON_KEYS)
  MENU_TOUCH_BUTTON_LABELS = ["VIEW", "SAVE", "QUIT", "BACK"] unless const_defined?(:MENU_TOUCH_BUTTON_LABELS)

  def touch_button_labels
    return MENU_TOUCH_BUTTON_LABELS if @is_touch_menu_shown

    labels = EDIT_TOUCH_BUTTON_LABELS.dup
    labels[0] = CHANNEL_LABELS[@channel_index]
    labels
  end

  def draw_touch_buttons
    labels = touch_button_labels
    labels_text = labels.join(",")
    return if labels_text == @pushed_touch_button_labels_text

    show_touch_button_column(labels)
    @pushed_touch_button_labels_text = labels_text
  end

  def run_with_soft_keyboard(&block)
    is_keyboard_opened = !Touch.bluetooth_keyboard_connected?
    Touch.open_keyboard if is_keyboard_opened
    push_touch_button_column(-1) if is_keyboard_opened
    result = block.call
    Touch.close_keyboard if is_keyboard_opened
    push_touch_button_column(-1) if is_keyboard_opened
    result
  end

  def read_pending_character
    character = nil
    STDIN.raw do
      character = STDIN.read_nonblock(1)
    end
    character
  end

  def read_widget_key(first_character)
    return "ENTER" if first_character == "\r" || first_character == "\n"
    return "BS" if first_character == "\b" || first_character == "\x7f"
    return first_character if first_character != "\e"

    sleep_ms 40
    return "ESC" if read_pending_character != "["

    final_character = read_pending_character
    while final_character && final_character.ord >= 0x20 && final_character.ord < 0x40
      final_character = read_pending_character
    end
    case final_character
    when "A" then "UP"
    when "B" then "DOWN"
    when "C" then "RIGHT"
    when "D" then "LEFT"
    else ""
    end
  end

  def apply_touch_event(event)
    if event[0] == "RELEASE"
      @drag_start_y = nil
      return nil
    end
    return nil if event[0] == "DRAG" && @preview
    return scroll_rows_by_drag(event[2]) if event[0] == "DRAG"
    return "\t" if @preview
    return press_touch_button(event[1]) if event[0] == "BUTTON"

    select_touched_row(event[1], event[2])
  end

  def press_touch_button(button_index)
    if @is_touch_menu_shown
      @is_touch_menu_shown = false
      key = MENU_TOUCH_BUTTON_KEYS[button_index]
      return key ? key : ""
    end

    key = EDIT_TOUCH_BUTTON_KEYS[button_index]
    return key if key

    @is_touch_menu_shown = true
    ""
  end

  def select_touched_row(x, y)
    return nil if x < 0 || x >= Display.width || y < Widget.body_top

    row_offset = (y - Widget.body_top) / ROW_HEIGHT
    row_index = @top_row_index + row_offset
    return nil if row_offset >= visible_row_count || row_index >= ROW_COUNT
    return "ENTER" if row_index == @row_index

    @row_index = row_index
    ""
  end

  def divide_toward_zero(dividend, divisor)
    return -(-dividend / divisor) if dividend < 0

    dividend / divisor
  end

  def scroll_rows_by_drag(y)
    unless @drag_start_y
      @drag_start_y = y
      @drag_start_top_row_index = @top_row_index
    end
    top_row_index = @drag_start_top_row_index - divide_toward_zero(y - @drag_start_y, ROW_HEIGHT)
    top_row_index = [[top_row_index, ROW_COUNT - visible_row_count].min, 0].max
    return nil if top_row_index == @top_row_index

    @top_row_index = top_row_index
    ""
  end
end
