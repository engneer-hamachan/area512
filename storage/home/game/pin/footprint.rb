class Breadboard
  def has_footprint?(kind)
    case kind
    when 3, 4, 6, 7, 11, 12, 13
      true
    else
      false
    end
  end

  def count_terminals(kind)
    case kind
    when 3, 6
      4
    when 4
      10
    when 7, 11, 13
      3
    when 12
      5
    else
      2
    end
  end

  def list_contact_terminal_pairs(kind)
    case kind
    when 0
      [[0, 1]]
    when 4
      [[2, 7]]
    when 6
      [[0, 2], [1, 3]]
    else
      []
    end
  end

  def compute_footprint_row_offset(kind, terminal_index)
    case kind
    when 4
      terminal_index < 5 ? 0 : 3
    when 6
      terminal_index / 2
    when 12
      terminal_index < 2 ? 0 : 3
    else
      0
    end
  end

  def compute_footprint_column_offset(kind, terminal_index)
    case kind
    when 4
      terminal_index % 5
    when 6
      terminal_index % 2 * 2
    when 12
      terminal_index < 2 ? terminal_index * 2 : terminal_index - 2
    else
      terminal_index
    end
  end

  def compute_footprint_hole_indices(kind, anchor_row, anchor_column)
    hole_indices = []
    i = 0
    while i < count_terminals(kind)
      row = anchor_row + compute_footprint_row_offset(kind, i)
      column = anchor_column + compute_footprint_column_offset(kind, i)
      return nil unless hole_exists?(row, column)
      hole_indices.push(build_hole_index(row, column))
      i += 1
    end
    hole_indices
  end
end
