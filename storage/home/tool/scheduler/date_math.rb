class Scheduler
  def leap?(year)
    (year % 4 == 0 && year % 100 != 0) || year % 400 == 0
  end

  def days_in_month(year, month)
    return 29 if month == 2 && leap?(year)
    return 28 if month == 2
    return 30 if month == 4 || month == 6 || month == 9 || month == 11
    31
  end

  def weekday(year, month, day)
    y = year
    m = month
    if m < 3
      m += 12
      y -= 1
    end
    k = y % 100
    j = y / 100
    h = (day + 13 * (m + 1) / 5 + k + k / 4 + j / 4 + 5 * j) % 7
    (h + 6) % 7
  end

  def date_key
    format_date(@year, @month, @day)
  end

  def format_date(year, month, day)
    ys = year.to_s
    ys = "0" + ys while ys.length < 4
    ms = month < 10 ? "0" + month.to_s : month.to_s
    ds = day < 10 ? "0" + day.to_s : day.to_s
    ys + "-" + ms + "-" + ds
  end

  def move_day(delta)
    step = delta < 0 ? -1 : 1
    remaining = delta < 0 ? -delta : delta
    while remaining > 0
      move_one_day(step)
      remaining -= 1
    end
    reset_event_cursor
  end

  def move_one_day(step)
    @day += step
    if @day < 1
      @month -= 1
      if @month < 1
        @month = 12
        @year -= 1
      end
      @day = days_in_month(@year, @month)
    elsif @day > days_in_month(@year, @month)
      @day = 1
      @month += 1
      if @month > 12
        @month = 1
        @year += 1
      end
    end
  end

  def move_month(delta)
    @month += delta
    if @month < 1
      @month = 12
      @year -= 1
    elsif @month > 12
      @month = 1
      @year += 1
    end
    max = days_in_month(@year, @month)
    @day = max if @day > max
  end
end
