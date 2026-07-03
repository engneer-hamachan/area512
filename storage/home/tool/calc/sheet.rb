class Calc
  def init_sheet
    @cells = {}
    init_values
    @row = 0
    @col = 0
    @top = 0
    @left = 0
    @msg = "Ready. Type e to edit a cell."
    @quit = false
    @editing = false
    @edit_buf = ""
  end

  def cell_name(c, r)
    (65 + c.to_i).chr + (r.to_i + 1).to_s
  end

  def selected_name
    cell_name(@col, @row)
  end

  def cell_at(c, r)
    return "" if r < 0 || r >= ROWS || c < 0 || c >= COLS
    v = @cells[cell_key(c, r)]
    v ? v : ""
  end

  def set_cell(c, r, text)
    return if r < 0 || r >= ROWS || c < 0 || c >= COLS
    k = cell_key(c, r)
    if text.length == 0
      @cells.delete(k)
    else
      @cells[k] = text
    end
    @dirty = true
  end

  def cell_key(c, r)
    r * COLS + c
  end

  def move(dc, dr)
    @col += dc
    @row += dr
    @col = 0 if @col < 0
    @row = 0 if @row < 0
    @col = COLS - 1 if @col >= COLS
    @row = ROWS - 1 if @row >= ROWS
    adjust_scroll
  end

  def adjust_scroll
    cols = visible_cols
    rows = visible_rows
    @left = @col if @col < @left
    @top = @row if @row < @top
    @left = @col - cols + 1 if @col >= @left + cols
    @top = @row - rows + 1 if @row >= @top + rows
    @left = 0 if @left < 0
    @top = 0 if @top < 0
  end

  def visible_cols
    VIEW_COLS
  end

  def visible_rows
    n = (Display.height - TOP_H) / ROW_H
    n < 1 ? 1 : n
  end

  def cell_w
    w = (Display.width - HEAD_W) / VIEW_COLS
    w < 1 ? 1 : w
  end

  def display_value(c, r)
    recalc_sheet if @dirty
    raw = cell_at(c, r)
    if raw.start_with?("=")
      v = value_at(c, r)
      v ? format_value(v) : "#ERR"
    else
      raw
    end
  end

  def format_value(v)
    return v if v.is_a?(String)
    f = "#{v}".to_f
    i = f.to_i
    f == i ? i.to_s : f.to_s
  end

  def clip(s, w)
    s = "#{s}"
    return s if s.length <= w
    s[0, w]
  end
end
