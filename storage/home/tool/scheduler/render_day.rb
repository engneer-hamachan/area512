class Scheduler
  def draw_day
    draw_header("DAILY", date_key)
    list = day_events
    if list.empty?
      @sp.text(8, 38, "No schedule", C_TEXT)
      @sp.text(8, 54, "Press a to add", C_AMBER)
      return
    end
    row = 0
    while row < 6 && @event_top + row < list.length
      index = @event_top + row
      event = list[index]
      y = 22 + row * 16
      if index == @event_cursor
        @sp.fill_rect(3, y - 2, W - 6, 15, C_DARK)
        @sp.rect(3, y - 2, W - 6, 15, C_GOLD)
      end
      color = index == @event_cursor ? C_GOLD : C_TEXT
      @sp.text(7, y, event[0], C_AMBER)
      @sp.text(45, y, clip_text(event[1], 188), color)
      row += 1
    end
  end
end
