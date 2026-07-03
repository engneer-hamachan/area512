class Writer
  def draw
    @sp.fill(C_APP)
    draw_toolbar
    draw_page
    draw_status
    @sp.push(0, 0)
  end

  def draw_toolbar
    @sp.fill_rect(0, 0, Display.width, TOOL_H, C_TOOL)
    button(4, "^S Save", C_ACCENT, 48)
    button(56, "^O Open", C_ACCENT_2, 48)
    button(108, "^K", C_DIM, 22)
    button(134, "^J", C_DIM, 22)
    @sp.text(160, 7, "^Spc IME", C_DIM)
  end

  def button(x, label, color, w)
    @sp.fill_rect(x, 3, w, 16, C_PAGE)
    @sp.rect(x, 3, w, 16, C_TOOL_DARK)
    @sp.text(x + 3, 5, label, color)
  end

  def draw_page
    x = MARGIN + 5
    y = TOOL_H + MARGIN
    w = Display.width - MARGIN * 2 - 10
    h = Display.height - TOOL_H - STATUS_H - MARGIN * 2
    @sp.fill_rect(x + 3, y + 3, w, h, 0x9aa3ad)
    @sp.fill_rect(x, y, w, h, C_PAGE)
    @sp.rect(x, y, w, h, C_PAGE_EDGE)
    draw_ruler(x, y, w)
    @sp.line(x + PAGE_PAD - 4, y + 12, x + PAGE_PAD - 4, y + h - 4, C_MARGIN_LINE)
    draw_lines(x + PAGE_PAD, y + PAGE_PAD + 8, w - PAGE_PAD * 2)
  end

  def draw_ruler(x, y, w)
    ry = y + 4
    @sp.line(x + PAGE_PAD, ry, x + w - PAGE_PAD, ry, C_RULE)
    px = x + PAGE_PAD
    while px < x + w - PAGE_PAD
      @sp.line(px, ry, px, ry + 3, C_RULE)
      px += 20
    end
  end

  def draw_lines(x, y, w)
    r = @top
    yy = y
    rows = visible_rows
    while r < @top + rows && r < @lines.length
      line = @lines[r]
      line = "" unless line.is_a?(String)
      @sp.line(x, yy + LINE_H - 1, x + w, yy + LINE_H - 1, 0xf0f2f4)
      @sp.text(x, yy, clip_px(line, w), C_TEXT)
      draw_cursor(x, yy, w) if r == @row
      yy += LINE_H
      r += 1
    end
    while yy < Display.height - STATUS_H - MARGIN
      @sp.line(x, yy + LINE_H - 1, x + w, yy + LINE_H - 1, 0xf0f2f4)
      yy += LINE_H
    end
  end

  def draw_cursor(x, y, w)
    cx = x + text_width(current_line, @col)
    max = x + w
    cx = max if cx > max
    @sp.line(cx, y - 1, cx, y + LINE_H - 2, C_CURSOR)
  end

  def draw_status
    y = Display.height - STATUS_H
    @sp.fill_rect(0, y, Display.width, STATUS_H, C_STATUS)
    pos = (@row + 1).to_s + ":" + (@col + 1).to_s
    @sp.text(4, y + 3, DOC_NAME + dirty_mark, C_STATUS_TEXT)
    @sp.text(72, y + 3, pos + " " + @msg, C_STATUS_TEXT)
  end
end
