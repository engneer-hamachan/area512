class Bomb
  def draw_cell(sp, x, y)
    px = OX + x * CW
    py = OY + y * CH
    if @open[y][x]
      sp.fill_rect(px, py, CW - 1, CH - 1, C_OPEN)
      if @mines[y][x]
        sp.text(px + 3, py - 1, "*", C_MINE)
      else
        n = @nums[y][x]
        if n > 0
          color = C_TEXT
          case n
          when 1
            color = COL_GOLD
          when 2
            color = COL_AMBER
          when 3
            color = C_FLAG
          when 4
            color = C_DIM
          when 5
            color = C_CURSOR
          when 6
            color = C_MINE
          when 7
            color = C_TEXT
          else
            color = C_LOSE
          end
          sp.text(px + 3, py - 1, "#{n}", color)
        end
      end
    else
      sp.fill_rect(px, py, CW - 1, CH - 1, C_CLOSED)
      sp.text(px + 3, py - 1, "F", C_FLAG) if @flag[y][x]
    end
    sp.rect(px, py, CW - 1, CH - 1, C_GRID)
    sp.rect(px, py, CW - 1, CH - 1, C_CURSOR) if x == @cx && y == @cy
  end

  def status_color
    return C_LOSE if @dead
    return C_WIN if @win
    C_TEXT
  end

  def draw
    sp = @sp
    sp.fill(C_BG)
    sp.fill_rect(0, 0, Display.width, 14, C_PANEL)
    sp.text(4, 2, "MINES #{@flags}/#{MINES}", status_color)
    y = 0
    while y < MH
      x = 0
      while x < MW
        draw_cell(sp, x, y)
        x += 1
      end
      y += 1
    end
    sp.text(4, 124, "#{@msg}", C_DIM)
    sp.push(0, 0)
  end

  # Center a vertical list of [text, color] lines on the screen.
  # ASCII glyphs are 6px wide in efontJA_12; "" rows act as spacers.
  def draw_centered(sp, lines, lh)
    y = (Display.height - lines.length * lh) / 2
    lines.each do |line|
      s = "#{line[0]}"
      sp.text((Display.width - s.length * 6) / 2, y, s, line[1])
      y += lh
    end
  end

  def draw_end
    sp = @sp
    sp.fill(C_BG)
    draw_centered(sp, [
      [@win ? "Cleared!" : "Bomb", status_color],
      ["", C_TEXT],
      ["Opened #{@opened}/#{MW * MH - MINES}", C_TEXT],
      ["Press any key.", C_DIM]
    ], CH + 4)
    sp.push(0, 0)
  end
end
