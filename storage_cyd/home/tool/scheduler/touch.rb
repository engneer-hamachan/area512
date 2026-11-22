require 'area512-touch'

class Scheduler
  # The window's left edge inside the Filer's frame, so the touch buttons get
  # the room right of the window (Display.move_window).
  TOUCH_WINDOW_LEFT = 12 unless const_defined?(:TOUCH_WINDOW_LEFT)
  CALENDAR_TOUCH_BUTTONS = [
    ["<", "["], [">", "]"], ["Open", "\r"], ["Save", "s"], ["Quit", "q"]
  ] unless const_defined?(:CALENDAR_TOUCH_BUTTONS)
  DAY_TOUCH_BUTTONS = [
    ["Back", "c"], ["Add", "a"], ["Del", "d"], ["Save", "s"], ["Quit", "q"]
  ] unless const_defined?(:DAY_TOUCH_BUTTONS)
  CALENDAR_DAY_TOP = 31 unless const_defined?(:CALENDAR_DAY_TOP)
  CALENDAR_CELL_WIDTH = 34 unless const_defined?(:CALENDAR_CELL_WIDTH)
  CALENDAR_CELL_HEIGHT = 14 unless const_defined?(:CALENDAR_CELL_HEIGHT)
  EVENT_ROW_TOP = 20 unless const_defined?(:EVENT_ROW_TOP)
  EVENT_ROW_HEIGHT = 16 unless const_defined?(:EVENT_ROW_HEIGHT)
  EVENT_VISIBLE_ROW_COUNT = 6 unless const_defined?(:EVENT_VISIBLE_ROW_COUNT)

  def push_touch_buttons(buttons)
    labels = []
    index = 0
    while index < buttons.length
      labels.push(buttons[index][0])
      index += 1
    end
    show_touch_button_column(labels)
    @pushed_touch_buttons = buttons
  end

  def discard_touch_events
    event = read_touch_event
    event = read_touch_event while event
  end

  def read_touch_key
    event = read_touch_event
    return nil unless event
    kind = event[0]
    x = event[1]
    y = event[2]
    if kind == "DRAG"
      scroll_events_by_drag(y) if @screen == :day
      return nil
    end
    @drag_y = nil
    return @pushed_touch_buttons[x][1] if kind == "BUTTON"
    return nil if kind != "TAP"
    return nil if x < 0 || x >= W
    @screen == :calendar ? select_or_open_day_at(x, y) : select_event_at(y)
  end

  def select_or_open_day_at(x, y)
    column = x / CALENDAR_CELL_WIDTH
    day = (y - CALENDAR_DAY_TOP) / CALENDAR_CELL_HEIGHT * WDAYS.length + column - weekday(@year, @month, 1) + 1
    return nil if y < CALENDAR_DAY_TOP || column >= WDAYS.length || day < 1 || day > days_in_month(@year, @month)
    return "\r" if day == @day
    @day = day
    reset_event_cursor
    draw
    nil
  end

  def select_event_at(y)
    row = (y - EVENT_ROW_TOP) / EVENT_ROW_HEIGHT
    return nil if y < EVENT_ROW_TOP || row >= EVENT_VISIBLE_ROW_COUNT || @event_top + row >= day_events.length
    @event_cursor = @event_top + row
    draw
    nil
  end

  def scroll_events_by_drag(y)
    @drag_y = y unless @drag_y
    distance = @drag_y - y
    scroll_row_count = distance.abs / EVENT_ROW_HEIGHT
    return if scroll_row_count == 0
    scroll_row_count = -scroll_row_count if distance < 0
    @drag_y -= scroll_row_count * EVENT_ROW_HEIGHT
    maximum_top = day_events.length - EVENT_VISIBLE_ROW_COUNT
    top = @event_top + scroll_row_count
    top = maximum_top if top > maximum_top
    top = 0 if top < 0
    return if top == @event_top
    @event_top = top
    @event_cursor = top if @event_cursor < top
    @event_cursor = top + EVENT_VISIBLE_ROW_COUNT - 1 if @event_cursor >= top + EVENT_VISIBLE_ROW_COUNT
    draw
  end
end
