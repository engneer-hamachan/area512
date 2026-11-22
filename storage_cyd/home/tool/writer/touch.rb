require 'area512-touch'

class Writer
  # The window's left edge inside the Filer's frame, so the touch buttons get
  # the room right of the window (Display.move_window).
  TOUCH_WINDOW_LEFT = 12 unless const_defined?(:TOUCH_WINDOW_LEFT)
  OPEN_KEY = "\x0f" unless const_defined?(:OPEN_KEY)
  SAVE_KEY = "\x13" unless const_defined?(:SAVE_KEY)
  TOUCH_BUTTONS = [
    ["Kbd", nil], ["Open", OPEN_KEY], ["Save", SAVE_KEY], ["Quit", "\x11"]
  ] unless const_defined?(:TOUCH_BUTTONS)
  LIST_TOUCH_BUTTONS = [["Back", "ESC"]] unless const_defined?(:LIST_TOUCH_BUTTONS)
  WIDGET_LIST_MAXIMUM_COUNT = 128 unless const_defined?(:WIDGET_LIST_MAXIMUM_COUNT)

  def push_touch_buttons(buttons, highlighted_index)
    labels = []
    index = 0
    while index < buttons.length
      labels.push(buttons[index][0])
      index += 1
    end
    show_touch_button_column(labels)
    set_touch_button_on(highlighted_index, true) if highlighted_index
    @pushed_touch_buttons = buttons
  end

  def find_touch_button(buttons, event)
    return nil if event[0] != "BUTTON" || event[1] >= buttons.length
    buttons[event[1]]
  end

  def read_pending_chars(count)
    chars = nil
    STDIN.raw { chars = STDIN.read_nonblock(count) }
    chars
  end

  def toggle_keyboard
    @is_keyboard_open ? Touch.close_keyboard : Touch.open_keyboard
    @is_keyboard_open = !@is_keyboard_open
    @pushed_touch_buttons = nil
    draw
  end

  def run_with_soft_keyboard(&block)
    is_keyboard_opened = !@is_keyboard_open && !Touch.bluetooth_keyboard_connected?
    Touch.open_keyboard if is_keyboard_opened
    push_touch_button_column(-1) if is_keyboard_opened
    result = block.call
    Touch.close_keyboard if is_keyboard_opened
    push_touch_button_column(-1) if is_keyboard_opened
    result
  end

  def read_touch_input
    event = read_touch_event
    return nil unless event
    kind = event[0]
    x = event[1]
    y = event[2]
    button = find_touch_button(TOUCH_BUTTONS, event)
    if button
      toggle_keyboard if button[1].nil?
      return button[1]
    end
    if kind == "DRAG"
      scroll_page_by_drag(y)
      return nil
    end
    @drag_y = nil
    return nil if kind != "TAP"
    body_top = Widget.body_top
    return nil if x < 0 || x >= Display.width || y < body_top || y >= body_top + Widget.body_height
    move_cursor_to_tap(x, y)
    nil
  end

  def find_page_segment(page_row)
    row = @top
    segment_index = @top_segment_index
    while page_row > 0
      if segment_index + 1 < @line_segment_counts[row]
        segment_index += 1
      elsif row + 1 < @lines.length
        row += 1
        segment_index = 0
      else
        return nil
      end
      page_row -= 1
    end
    [row, segment_index]
  end

  def count_chars_before_x(segment, x)
    byte_index = 0
    left_char_count = 0
    width = 0
    while byte_index < segment.length
      glyph_width = glyph_width_at(segment, byte_index)
      break if width + glyph_width / 2 > x
      width += glyph_width
      byte_index += utf8_step(segment, byte_index)
      left_char_count += 1
    end
    left_char_count
  end

  def move_cursor_to_tap(x, y)
    page_row = (y - Widget.body_top - PAGE_PAD) / LINE_H
    page_row = 0 if page_row < 0
    page_row = @page_segments.length - 1 if page_row >= @page_segments.length
    page_segment = find_page_segment(page_row)
    segment_col = count_chars_before_x(@page_segments[page_row], x - PAGE_PAD)
    move_cursor_to_segment(page_segment[0], page_segment[1], segment_col)
    draw
  end

  def scroll_up_one_segment
    if @top_segment_index > 0
      @top_segment_index -= 1
    else
      @top -= 1
      @top_segment_index = @line_segment_counts[@top] - 1
    end
  end

  def move_cursor_onto_page
    if cursor_above_page?
      move_cursor_to_segment(@top, @top_segment_index, @cursor_segment_col)
      return
    end
    last_page_segment = find_page_segment(visible_rows - 1)
    return unless last_page_segment
    last_row = last_page_segment[0]
    last_segment_index = last_page_segment[1]
    return if @row < last_row || (@row == last_row && @cursor_segment_index <= last_segment_index)
    move_cursor_to_segment(last_row, last_segment_index, @cursor_segment_col)
  end

  def scroll_page_by_drag(y)
    @drag_y = y unless @drag_y
    dragged_from_y = @drag_y
    while @drag_y - y >= LINE_H && find_page_segment(visible_rows)
      scroll_down_one_segment
      @drag_y -= LINE_H
    end
    while y - @drag_y >= LINE_H && (@top > 0 || @top_segment_index > 0)
      scroll_up_one_segment
      @drag_y += LINE_H
    end
    return if @drag_y == dragged_from_y
    move_cursor_onto_page
    draw
  end

  def reset_list_position
    @list_count = list_doc_names.length
    @list_count = WIDGET_LIST_MAXIMUM_COUNT if @list_count > WIDGET_LIST_MAXIMUM_COUNT
    @list_index = 0
    @list_top = 0
    @queued_list_keys = []
  end

  def move_list_position(key)
    @list_index -= 1 if key == "UP" || key == "k" || key == ";"
    @list_index += 1 if key == "DOWN" || key == "j" || key == "."
    @list_index = @list_count - 1 if @list_index >= @list_count
    @list_index = 0 if @list_index < 0
    row_count = Widget.body_height / Widget.row_height
    @list_top = @list_index if @list_index < @list_top
    @list_top = @list_index - row_count + 1 if @list_index >= @list_top + row_count
  end

  def queue_keys_for_list_tap(x, y)
    body_top = Widget.body_top
    row = (y - body_top) / Widget.row_height
    target_index = @list_top + row
    return nil if x < 0 || x >= Display.width || y < body_top
    return nil if row >= Widget.body_height / Widget.row_height || target_index >= @list_count
    step_key = target_index < @list_index ? "UP" : "DOWN"
    step_count = (target_index - @list_index).abs
    while step_count > 0
      @queued_list_keys.push(step_key)
      step_count -= 1
    end
    @queued_list_keys.push("ENTER")
    @queued_list_keys.shift
  end

  def convert_char_to_widget_key(char)
    return "ENTER" if char == "\r" || char == "\n"
    return "BS" if char == "\b" || char == "\x7f"
    return char if char != "\e"
    sleep_ms 40
    sequence = read_pending_chars(2)
    return "ESC" if sequence.nil? || sequence[0] != "["
    final_char = sequence[1]
    return "UP" if final_char == "A"
    return "DOWN" if final_char == "B"
    return "RIGHT" if final_char == "C"
    return "LEFT" if final_char == "D"
    ""
  end

  def wait_list_key
    while true
      char = read_pending_chars(1)
      return convert_char_to_widget_key(char) if char
      event = read_touch_event
      button = event ? find_touch_button(LIST_TOUCH_BUTTONS, event) : nil
      return button[1] if button
      if event && event[0] == "TAP"
        key = queue_keys_for_list_tap(event[1], event[2])
        return key if key
      end
      sleep_ms 10
    end
  end
end
