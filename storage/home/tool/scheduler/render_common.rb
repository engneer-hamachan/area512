class Scheduler
  def draw
    @sp.fill(C_BG)
    @screen == :calendar ? draw_calendar : draw_day
    draw_footer
    @sp.push(0, 0)
  end

  def draw_header(title, right)
    @sp.rect(0, 0, W - 1, 17, C_AMBER)
    @sp.text(5, 3, title, C_GOLD)
    @sp.text(158, 3, right, C_TEXT)
  end

  def draw_footer
    @sp.line(0, 121, W - 1, 121, C_AMBER)
    @sp.text(3, 124, clip_text(@msg, 234), C_TEXT)
  end

  def draw_entry(label, text)
    draw
    @sp.fill_rect(0, 85, W, 36, C_DARK)
    @sp.rect(0, 85, W - 1, 35, C_AMBER)
    @sp.text(4, 89, label, C_TEXT)
    shown = clip_text(text, 226)
    @sp.text(4, 105, shown + "_", C_GOLD)
    @sp.push(0, 0)
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
