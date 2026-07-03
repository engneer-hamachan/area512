class Writer
  def init_doc
    @lines = [""]
    @row = 0
    @col = 0
    @top = 0
    @msg = "Ready"
    @quit = false
    @jp_mode = false
    @dirty = false
  end

  def current_line
    line = @lines[@row]
    line.is_a?(String) ? line : ""
  end

  def set_current_line(text)
    @lines[@row] = text
  end

  def char_count(text)
    s = "#{text}"
    i = 0
    n = 0
    while i < s.length
      i += utf8_step(s, i)
      n += 1
    end
    n
  end

  def left_text(text, count)
    s = "#{text}"
    i = 0
    n = 0
    while i < s.length && n < count
      i += utf8_step(s, i)
      n += 1
    end
    part = s[0, i]
    part.is_a?(String) ? part : ""
  end

  def right_text(text, count)
    s = "#{text}"
    i = 0
    n = 0
    while i < s.length && n < count
      i += utf8_step(s, i)
      n += 1
    end
    part = s[i, s.length - i]
    part.is_a?(String) ? part : ""
  end

  def utf8_step(text, pos)
    b = text.getbyte(pos)
    return 1 unless b.is_a?(Integer)
    return 1 if b < 0x80
    return 2 if b < 0xe0
    return 3 if b < 0xf0
    4
  end

  def glyph_width_at(text, pos)
    b = text.getbyte(pos)
    return GLYPH_W unless b.is_a?(Integer)
    return GLYPH_W if b < 0x80
    WIDE_W
  end

  def text_width(text, count)
    s = "#{text}"
    i = 0
    n = 0
    w = 0
    while i < s.length && n < count
      w += glyph_width_at(s, i)
      i += utf8_step(s, i)
      n += 1
    end
    w
  end

  def clip_px(text, max_w)
    s = "#{text}"
    i = 0
    w = 0
    while i < s.length
      cw = glyph_width_at(s, i)
      break if w + cw > max_w
      w += cw
      i += utf8_step(s, i)
    end
    part = s[0, i]
    part.is_a?(String) ? part : ""
  end

  def clamp_cursor
    @row = 0 if @row < 0
    @row = @lines.length - 1 if @row >= @lines.length
    @col = 0 if @col < 0
    len = char_count(current_line)
    @col = len if @col > len
    adjust_scroll
  end

  def visible_rows
    h = Display.height - TOOL_H - STATUS_H - MARGIN * 2 - PAGE_PAD * 2 - 8
    n = h / LINE_H
    n < 1 ? 1 : n
  end

  def adjust_scroll
    rows = visible_rows
    @top = @row if @row < @top
    @top = @row - rows + 1 if @row >= @top + rows
    @top = 0 if @top < 0
  end

  def dirty_mark
    @dirty ? "*" : " "
  end
end
