require 'io/console'
require 'area512-touch'

class Solitair
  # The window's left edge inside the Filer's frame, so the touch buttons get
  # the room right of the window (Display.move_window).
  TOUCH_WINDOW_LEFT = 12 unless const_defined?(:TOUCH_WINDOW_LEFT)
  TABLEAU_LABEL_Y = TABLEAU_Y - CH - 4 unless const_defined?(:TABLEAU_LABEL_Y)
  TOUCH_BUTTON_LABELS = ["Auto", "New", "Quit"] unless const_defined?(:TOUCH_BUTTON_LABELS)
  TOUCH_BUTTON_KEYS = ["a", "n", "q"] unless const_defined?(:TOUCH_BUTTON_KEYS)
  TOUCH_POLL_INTERVAL_MS = 10 unless const_defined?(:TOUCH_POLL_INTERVAL_MS)

  def draw_touch_buttons
    show_touch_button_column(TOUCH_BUTTON_LABELS)
  end

  def find_column_index(x)
    return -1 if x < board_x
    column_index = (x - board_x) / COL_W
    column_index > 6 ? -1 : column_index
  end

  def tableau_area?(y)
    y >= board_y + TABLEAU_LABEL_Y && y < board_y + BOARD_H
  end

  def find_tableau_card_index(pile, column_index, y)
    row_index = (y - board_y - TABLEAU_Y) / STACK_STEP
    row_index = 0 if row_index < 0
    last_index = pile.length - 1
    return last_index if row_index >= VISIBLE_ROWS - 1 || row_index >= last_index
    return row_index if pile.length <= VISIBLE_ROWS || row_index == 0
    @scroll[column_index] + row_index
  end

  def apply_top_slot_tap(slot_index)
    if slot_index == 0
      @cursor = 0
      return "d"
    end
    if slot_index <= 2
      @cursor = 1
      return "f" if @sel == 1
      clear_sel
      return "0"
    end
    @cursor = slot_index - 1
    @sel >= 0 ? "f" : " "
  end

  def apply_tableau_tap(column_index, y)
    @cursor = column_index + 6
    return "f" if @sel == @cursor
    number_key = "#{column_index + 1}"
    return number_key if @sel >= 0
    pile = @tab[column_index]
    return number_key if pile.empty? || !face_up?(pile[pile.length - 1])
    card_index = find_tableau_card_index(pile, column_index, y)
    card_index = first_face_up_index(pile) unless face_up?(pile[card_index])
    return number_key unless can_stack_from?(pile, card_index)
    @sel = @cursor
    @sel_idx = card_index - 1
    "j"
  end

  def apply_tap(x, y)
    return "" if @quit
    column_index = find_column_index(x)
    return nil if column_index < 0 || y < board_y
    return apply_tableau_tap(column_index, y) if tableau_area?(y)
    return nil if y >= board_y + TABLEAU_LABEL_Y
    apply_top_slot_tap(column_index)
  end

  def scroll_by_drag(x, y)
    if @last_scroll_y.nil?
      @last_scroll_y = y
      @drag_column_index = tableau_area?(y) ? find_column_index(x) : -1
      return
    end
    return if @drag_column_index < 0
    scroll_count = 0
    @cursor = @drag_column_index + 6
    while @last_scroll_y - y >= STACK_STEP
      scroll_col(1)
      @last_scroll_y -= STACK_STEP
      scroll_count += 1
    end
    while y - @last_scroll_y >= STACK_STEP
      scroll_col(-1)
      @last_scroll_y += STACK_STEP
      scroll_count += 1
    end
    draw if scroll_count > 0
  end

  def apply_touch_event(event)
    return apply_tap(event[1], event[2]) if event[0] == "TAP"
    if event[0] == "BUTTON"
      return "" if @quit
      return TOUCH_BUTTON_KEYS[event[1]]
    end
    if event[0] == "DRAG"
      scroll_by_drag(event[1], event[2]) unless @quit
    else
      @last_scroll_y = nil
    end
    nil
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
        key = apply_touch_event(event)
        return key if key
        event = read_touch_event
      end
      sleep_ms TOUCH_POLL_INTERVAL_MS
    end
  end
end
