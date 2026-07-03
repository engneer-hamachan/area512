class Bomb
  def make_bool_grid
    g = []
    y = 0
    while y < MH
      row = []
      x = 0
      while x < MW
        row.push(false)
        x += 1
      end
      g.push(row)
      y += 1
    end
    g
  end

  def make_num_grid
    g = []
    y = 0
    while y < MH
      row = []
      x = 0
      while x < MW
        row.push(0)
        x += 1
      end
      g.push(row)
      y += 1
    end
    g
  end

  def in_bounds?(x, y)
    x >= 0 && x < MW && y >= 0 && y < MH
  end

  def init_game
    @mines = make_bool_grid
    @open = make_bool_grid
    @flag = make_bool_grid
    @nums = make_num_grid
    @cx = MW / 2
    @cy = MH / 2
    @opened = 0
    @flags = 0
    @dead = false
    @win = false
    @quit = false
    @started = false
    @msg = "Space open, f flag, r new, q quit"
  end

  def place_mines(sx, sy)
    placed = 0
    while placed < MINES
      x = rnd(MW)
      y = rnd(MH)
      next if @mines[y][x]
      next if x == sx && y == sy
      @mines[y][x] = true
      placed += 1
    end
    calc_numbers
    @started = true
  end

  def count_around(x, y)
    n = 0
    dy = -1
    while dy <= 1
      dx = -1
      while dx <= 1
        if dx != 0 || dy != 0
          xx = x + dx
          yy = y + dy
          n += 1 if in_bounds?(xx, yy) && @mines[yy][xx]
        end
        dx += 1
      end
      dy += 1
    end
    n
  end

  def calc_numbers
    y = 0
    while y < MH
      x = 0
      while x < MW
        @nums[y][x] = count_around(x, y)
        x += 1
      end
      y += 1
    end
  end
end
