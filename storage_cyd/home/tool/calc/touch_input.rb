require 'area512-touch'

class Calc
  # The window's left edge inside the Filer's frame, so the touch buttons get
  # the room right of the window (Display.move_window).
  TOUCH_WINDOW_LEFT = 12 unless const_defined?(:TOUCH_WINDOW_LEFT)
  TOUCH_BUTTON_LABELS = ["KB", "CLEAR", "GOTO", "SAVE", "OPEN", "QUIT"] unless const_defined?(:TOUCH_BUTTON_LABELS)
  TOUCH_BUTTON_KEYS = [nil, "c", "g", "s", "o", "q"] unless const_defined?(:TOUCH_BUTTON_KEYS)
  KEYBOARD_BUTTON_INDEX = 0 unless const_defined?(:KEYBOARD_BUTTON_INDEX)
  QUIT_BUTTON_INDEX = 5 unless const_defined?(:QUIT_BUTTON_INDEX)

  def draw_touch_buttons
    unless @is_touch_button_drawn
      show_touch_button_column(TOUCH_BUTTON_LABELS)
      @is_touch_button_drawn = true
    end
    set_touch_button_on(KEYBOARD_BUTTON_INDEX, @keyboard_open == true)
  end

  def open_keyboard
    Touch.open_keyboard
    @keyboard_open = true
    set_touch_button_on(KEYBOARD_BUTTON_INDEX, true)
  end

  def close_keyboard
    Touch.close_keyboard
    @keyboard_open = false
    set_touch_button_on(KEYBOARD_BUTTON_INDEX, false)
  end

  def run_with_soft_keyboard(&block)
    is_keyboard_opened = !@keyboard_open && !Touch.bluetooth_keyboard_connected?
    Touch.open_keyboard if is_keyboard_opened
    push_touch_button_column(-1) if is_keyboard_opened
    result = block.call
    Touch.close_keyboard if is_keyboard_opened
    push_touch_button_column(-1) if is_keyboard_opened
    result
  end

  def discard_touch_events
    while read_touch_event
    end
  end

  def apply_touch_event(event)
    if event[0] == "RELEASE"
      @drag_start_y = nil
      return nil
    end
    return scroll_sheet_by_drag(event[1], event[2]) if event[0] == "DRAG"
    return press_touch_button(event[1]) if event[0] == "BUTTON"

    select_touched_cell(event[1], event[2])
  end

  def press_touch_button(button_index)
    key = TOUCH_BUTTON_KEYS[button_index]
    return nil if key && @editing
    return key if key

    if @keyboard_open
      close_keyboard
    else
      open_keyboard
    end
    ""
  end

  def select_touched_cell(x, y)
    return nil if @editing || x < HEAD_W || x >= Display.width || y < TOP_H

    column_index = @left + (x - HEAD_W) / cell_w
    row_index = @top + (y - TOP_H) / ROW_H
    return nil if column_index >= @left + visible_cols || column_index >= COLS
    return nil if row_index >= @top + visible_rows || row_index >= ROWS

    if column_index == @col && row_index == @row
      open_keyboard unless @keyboard_open || Touch.bluetooth_keyboard_connected?
      return "e"
    end
    @col = column_index
    @row = row_index
    ""
  end

  def divide_toward_zero(dividend, divisor)
    return -(-dividend / divisor) if dividend < 0

    dividend / divisor
  end

  def scroll_sheet_by_drag(x, y)
    unless @drag_start_y
      @drag_start_x = x
      @drag_start_y = y
      @drag_start_top = @top
      @drag_start_left = @left
    end
    top = @drag_start_top - divide_toward_zero(y - @drag_start_y, ROW_H)
    top = [[top, ROWS - visible_rows].min, 0].max
    left = @drag_start_left - divide_toward_zero(x - @drag_start_x, cell_w)
    left = [[left, COLS - visible_cols].min, 0].max
    return nil if top == @top && left == @left

    @top = top
    @left = left
    ""
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

  def start_tracking_sheet_list_cursor
    @sheet_list_index = 0
    @sheet_list_top = 0
    @sheet_count = list_sheet_names.length
  end

  def track_sheet_list_cursor(key)
    if key == "ESC" || (key == "ENTER" && @sheet_count > 0)
      @sheet_list_index = nil
      return
    end
    sheet_list_index = @sheet_list_index
    sheet_list_index -= 1 if key == "UP" || key == "k" || key == ";"
    sheet_list_index += 1 if key == "DOWN" || key == "j" || key == "."
    sheet_list_index = [[sheet_list_index, @sheet_count - 1].min, 0].max
    visible_sheet_count = Widget.body_height / Widget.row_height
    @sheet_list_top = sheet_list_index if sheet_list_index < @sheet_list_top
    @sheet_list_top = sheet_list_index - visible_sheet_count + 1 if sheet_list_index >= @sheet_list_top + visible_sheet_count
    @sheet_list_index = sheet_list_index
  end

  def step_toward_tapped_sheet
    return "UP" if @tapped_sheet_index < @sheet_list_index
    return "DOWN" if @tapped_sheet_index > @sheet_list_index

    @tapped_sheet_index = nil
    "ENTER"
  end

  def apply_sheet_list_touch_event(event)
    if event[0] == "BUTTON"
      return "ESC" if event[1] == QUIT_BUTTON_INDEX
      return nil
    end
    return nil if event[0] != "TAP"
    return nil if event[1] < 0 || event[1] >= Display.width || event[2] < Widget.body_top

    row_offset = (event[2] - Widget.body_top) / Widget.row_height
    sheet_index = @sheet_list_top + row_offset
    return nil if row_offset >= Widget.body_height / Widget.row_height || sheet_index >= @sheet_count

    @tapped_sheet_index = sheet_index
    step_toward_tapped_sheet
  end

  def read_sheet_list_input
    return step_toward_tapped_sheet if @tapped_sheet_index

    draw_touch_buttons
    while true
      character = read_pending_character
      return read_widget_key(character) if character

      event = read_touch_event
      if event
        key = apply_sheet_list_touch_event(event)
        return key if key
      else
        sleep_ms 10
      end
    end
  end
end
