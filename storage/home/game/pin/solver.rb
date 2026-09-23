class Breadboard
  def assign_nodes
    parent_net_of_net = build_parent_net_of_net
    node_index_of_net = Array.new(64, -1)
    @node_count = 0
    i = 0
    while i < @parts.length
      part = @parts[i]
      part.terminal_node_indices = []
      j = 0
      while j < part.terminal_hole_indices.length
        net = find_root_net(parent_net_of_net, find_base_net(part.terminal_hole_indices[j]))
        if node_index_of_net[net] < 0
          node_index_of_net[net] = @node_count
          @node_count += 1
        end
        part.terminal_node_indices.push(node_index_of_net[net])
        j += 1
      end
      i += 1
    end
    allocate_matrix
  end

  def build_parent_net_of_net
    parent_net_of_net = Array.new(64)
    i = 0
    while i < 64
      parent_net_of_net[i] = i
      i += 1
    end
    i = 0
    while i < @parts.length
      part = @parts[i]
      contact_terminal_pairs = list_contact_terminal_pairs(part.kind)
      j = 0
      while j < contact_terminal_pairs.length
        first_root_net = find_root_net(parent_net_of_net, find_base_net(part.terminal_hole_indices[contact_terminal_pairs[j][0]]))
        second_root_net = find_root_net(parent_net_of_net, find_base_net(part.terminal_hole_indices[contact_terminal_pairs[j][1]]))
        parent_net_of_net[first_root_net] = second_root_net
        j += 1
      end
      i += 1
    end
    parent_net_of_net
  end

  def find_root_net(parent_net_of_net, net)
    while parent_net_of_net[net] != net
      net = parent_net_of_net[net]
    end
    net
  end

  def allocate_matrix
    @matrix = []
    i = 0
    while i < @node_count
      @matrix.push(Array.new(@node_count, 0.0))
      i += 1
    end
    @right_side = Array.new(@node_count, 0.0)
    @node_voltages = Array.new(@node_count, 0.0)
  end

  def clear_matrix
    i = 0
    while i < @node_count
      row_values = @matrix[i]
      j = 0
      while j < @node_count
        row_values[j] = 0.0
        j += 1
      end
      row_values[i] = 0.000000001
      @right_side[i] = 0.0
      i += 1
    end
  end

  def stamp_conductance(first_node_index, second_node_index, conductance)
    first_row = @matrix[first_node_index]
    second_row = @matrix[second_node_index]
    first_row[first_node_index] += conductance
    second_row[second_node_index] += conductance
    first_row[second_node_index] -= conductance
    second_row[first_node_index] -= conductance
  end

  def stamp_source(positive_node_index, negative_node_index, voltage, resistance)
    conductance = 1.0 / resistance
    stamp_conductance(positive_node_index, negative_node_index, conductance)
    @right_side[positive_node_index] += conductance * voltage
    @right_side[negative_node_index] -= conductance * voltage
  end

  def stamp_contact(first_node_index, second_node_index)
    stamp_conductance(first_node_index, second_node_index, 1.0 / 0.01)
  end

  def stamp_controlled_current(collector_node_index, emitter_node_index, base_node_index, gain_conductance, offset_voltage)
    collector_row = @matrix[collector_node_index]
    emitter_row = @matrix[emitter_node_index]
    collector_row[base_node_index] += gain_conductance
    collector_row[emitter_node_index] -= gain_conductance
    emitter_row[base_node_index] -= gain_conductance
    emitter_row[emitter_node_index] += gain_conductance
    @right_side[collector_node_index] += gain_conductance * offset_voltage
    @right_side[emitter_node_index] -= gain_conductance * offset_voltage
  end

  def swap_matrix_rows(first_row_index, second_row_index)
    row_values = @matrix[first_row_index]
    @matrix[first_row_index] = @matrix[second_row_index]
    @matrix[second_row_index] = row_values
    right_value = @right_side[first_row_index]
    @right_side[first_row_index] = @right_side[second_row_index]
    @right_side[second_row_index] = right_value
  end

  def find_pivot_row_index(column)
    pivot_row_index = column
    largest_magnitude = @matrix[column][column].abs
    i = column + 1
    while i < @node_count
      if @matrix[i][column].abs > largest_magnitude
        largest_magnitude = @matrix[i][column].abs
        pivot_row_index = i
      end
      i += 1
    end
    pivot_row_index
  end

  def eliminate_below(column)
    pivot_row = @matrix[column]
    i = column + 1
    while i < @node_count
      target_row = @matrix[i]
      factor = target_row[column] / pivot_row[column]
      if factor != 0.0
        j = column
        while j < @node_count
          target_row[j] -= factor * pivot_row[j]
          j += 1
        end
        @right_side[i] -= factor * @right_side[column]
      end
      i += 1
    end
  end

  def solve_linear_system
    column = 0
    while column < @node_count
      pivot_row_index = find_pivot_row_index(column)
      swap_matrix_rows(column, pivot_row_index) if pivot_row_index != column
      eliminate_below(column)
      column += 1
    end
    i = @node_count - 1
    while i >= 0
      sum = @right_side[i]
      j = i + 1
      while j < @node_count
        sum -= @matrix[i][j] * @node_voltages[j]
        j += 1
      end
      @node_voltages[i] = sum / @matrix[i][i]
      i -= 1
    end
  end

  def read_terminal_voltage(part, terminal_index)
    @node_voltages[part.terminal_node_indices[terminal_index]]
  end
end
