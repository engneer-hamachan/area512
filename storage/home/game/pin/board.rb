class Breadboard
  def build_hole_index(row, column)
    row * 30 + column
  end

  def read_hole_row(hole_index)
    hole_index / 30
  end

  def read_hole_column(hole_index)
    hole_index % 30
  end

  def hole_exists?(row, column)
    row >= 0 && row < 14 && column >= 0 && column < 30
  end

  def compute_row_y(row)
    y = 21 + row * 7
    y += 4 if row >= 2
    y += 7 if row >= 7
    y += 4 if row >= 12
    y
  end

  def compute_column_x(column)
    18 + column * 7
  end

  def compute_hole_x(hole_index)
    compute_column_x(read_hole_column(hole_index))
  end

  def compute_hole_y(hole_index)
    compute_row_y(read_hole_row(hole_index))
  end

  def find_base_net(hole_index)
    row = read_hole_row(hole_index)
    column = read_hole_column(hole_index)
    return row if row < 2
    return 2 + row - 12 if row >= 12
    return 4 + column if row < 7
    4 + 30 + column
  end

  def format_hole_name(hole_index)
    row = read_hole_row(hole_index)
    column_number = read_hole_column(hole_index) + 1
    return "#{"abcdefghij"[row - 2]}#{column_number}" if row >= 2 && row < 12
    "#{find_rail_name(find_base_net(hole_index))} #{column_number}"
  end

  def find_cursor_hole_index
    build_hole_index(@cursor_row, @cursor_column)
  end

  def move_cursor(column_step, row_step)
    row = @cursor_row + row_step
    column = @cursor_column + column_step
    return unless hole_exists?(row, column)
    @cursor_row = row
    @cursor_column = column
  end
end
