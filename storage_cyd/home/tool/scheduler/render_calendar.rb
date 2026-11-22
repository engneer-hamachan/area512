class Scheduler
  def draw_calendar
    draw_header("SCHEDULER", MONTHS[@month - 1] + " " + @year.to_s)
    col = 0
    while col < 7
      color = col == 0 ? C_EMPHASIS : C_TEXT
      @sp.text(7 + col * 33, 21, WDAYS[col], color)
      col += 1
    end
    first = weekday(@year, @month, 1)
    max = days_in_month(@year, @month)
    day = 1
    while day <= max
      cell = first + day - 1
      x = (cell % 7) * 34
      y = 32 + (cell / 7) * 14
      selected = day == @day
      @sp.fill_rect(x + 1, y - 1, 31, 13, C_BOX) if selected
      @sp.rect(x + 1, y - 1, 31, 13, C_SELECTED) if selected
      color = selected ? C_SELECTED : C_TEXT
      color = C_EMPHASIS if weekday(@year, @month, day) == 0 && !selected
      @sp.text(x + 4, y, day.to_s, color)
      @sp.fill_circle(x + 27, y + 5, 2, C_EMPHASIS) if event_count(day) > 0
      day += 1
    end
  end
end
