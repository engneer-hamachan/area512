class Bomb
  def move_cursor(dx, dy)
    @cx += dx
    @cy += dy
    @cx = 0 if @cx < 0
    @cy = 0 if @cy < 0
    @cx = MW - 1 if @cx >= MW
    @cy = MH - 1 if @cy >= MH
  end

  def toggle_flag
    return if @dead || @win || @open[@cy][@cx]
    if @flag[@cy][@cx]
      @flag[@cy][@cx] = false
      @flags -= 1
    else
      @flag[@cy][@cx] = true
      @flags += 1
    end
    @msg = "Flagged #{@flags}/#{MINES}."
  end

  def open_cell(x, y)
    return unless in_bounds?(x, y)
    return if @open[y][x] || @flag[y][x]
    @open[y][x] = true
    @opened += 1
    return if @nums[y][x] != 0
    dy = -1
    while dy <= 1
      dx = -1
      while dx <= 1
        open_cell(x + dx, y + dy) if dx != 0 || dy != 0
        dx += 1
      end
      dy += 1
    end
  end

  def reveal_all
    y = 0
    while y < MH
      x = 0
      while x < MW
        @open[y][x] = true if @mines[y][x]
        x += 1
      end
      y += 1
    end
  end

  def check_win
    return if @dead
    if @opened >= MW * MH - MINES
      @win = true
      @msg = "Cleared! Press r for a new game."
    end
  end

  def open_cursor
    return if @dead || @win || @flag[@cy][@cx]
    place_mines(@cx, @cy) unless @started
    if @mines[@cy][@cx]
      @dead = true
      @msg = "Boom. Press r to try again."
      reveal_all
      return
    end
    open_cell(@cx, @cy)
    @msg = "Sweep carefully."
    check_win
  end
end
