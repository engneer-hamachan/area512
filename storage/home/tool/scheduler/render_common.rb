class Scheduler
  def draw
    while @sp.draw
      render
    end
  end

  def render
    @sp.fill(C_BACKGROUND)
    @screen == :calendar ? draw_calendar : draw_day
    draw_footer
  end

  def draw_header(title, right)
    @sp.rect(0, 0, W - 1, 17, C_BORDER)
    @sp.text(5, 3, title, C_EMPHASIS)
    @sp.text(158, 3, right, C_TEXT)
  end

  def draw_footer
    @sp.line(0, 121, W - 1, 121, C_BORDER)
    @sp.text(3, 124, clip_text(@msg, 234), C_TEXT)
  end

  def draw_entry(label, text)
    shown = clip_text(text, W - 14)
    entry_y = H - 50

    while @sp.draw
      render
      @sp.fill_rect(0, entry_y, W, 36, C_BOX)
      @sp.rect(0, entry_y, W - 1, 35, C_BORDER)
      @sp.text(4, entry_y + 4, label, C_TEXT)
      @sp.text(4, entry_y + 20, shown + "_", C_EMPHASIS)
    end
  end

  def clip(text, length)
    return text if text.length <= length
    cut = text[0, length - 1]
    cut.is_a?(String) ? cut + ">" : text
  end

  def event_count(day)
    list = @events[format_date(@year, @month, day)]
    list.is_a?(Array) ? list.length : 0
  end
end
