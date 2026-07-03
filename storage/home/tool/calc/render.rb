class Calc
  def draw
    @sp.fill(C_BG)
    draw_headers
    draw_cells
    @sp.push(0, 0)
  end

  def draw_headers
    y = TOP_H
    x = HEAD_W
    cw = cell_w
    @sp.rect(0, 0, HEAD_W, ROW_H, C_GRID)
    c = @left
    while c < @left + visible_cols && c < COLS
      @sp.rect(x, 0, cw, ROW_H, C_GRID)
      @sp.text(x + 3, 1, (65 + c).chr, C_TEXT)
      c += 1
      x += cw
    end
    r = @top
    while r < @top + visible_rows && r < ROWS
      @sp.rect(0, y, HEAD_W, ROW_H, C_GRID)
      @sp.text(2, y + 1, (r + 1).to_s, C_DIM)
      r += 1
      y += ROW_H
    end
  end

  def draw_cells
    r = @top
    y = TOP_H
    cw = cell_w
    while r < @top + visible_rows && r < ROWS
      c = @left
      x = HEAD_W
      while c < @left + visible_cols && c < COLS
        color = (r == @row && c == @col) ? C_EDIT : C_BG
        border = (r == @row && c == @col) ? C_SEL : C_GRID
        @sp.fill_rect(x, y, cw, ROW_H, color)
        @sp.rect(x, y, cw, ROW_H, border)
        txt = cell_text(c, r)
        @sp.text(x + 3, y + 1, txt, txt == "#ERR" ? C_ERR : C_TEXT)
        c += 1
        x += cw
      end
      r += 1
      y += ROW_H
    end
  end

  def cell_text(c, r)
    if @editing && c == @col && r == @row
      clip(@edit_buf + "_", 15)
    else
      clip(display_value(c, r), 15)
    end
  end
end
