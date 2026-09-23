class Breadboard
  def find_part_at(hole_index)
    i = 0
    while i < @parts.length
      return @parts[i] if @parts[i].terminal_hole_indices.include?(hole_index)
      i += 1
    end
    nil
  end

  def holes_free?(hole_indices)
    i = 0
    while i < hole_indices.length
      return false if find_part_at(hole_indices[i])
      j = i + 1
      while j < hole_indices.length
        return false if hole_indices[j] == hole_indices[i]
        j += 1
      end
      i += 1
    end
    true
  end

  def count_parts_of_kind(kind)
    count = 0
    i = 0
    while i < @parts.length
      count += 1 if @parts[i].kind == kind
      i += 1
    end
    count
  end

  def build_junctions(kind, color_index)
    case kind
    when 2
      [[0, 1, find_led_forward_voltage(color_index), 15.0]]
    when 3
      [[0, 1, 1.8, 15.0],
       [2, 1, 3.0, 15.0],
       [3, 1, 3.0, 15.0]]
    when 4
      build_seven_segment_junctions
    when 5
      [[0, 1, 0.7, 0.5]]
    when 9
      [[0, 1, 1.0, 150.0]]
    when 11
      [[1, 0, 0.7, 10.0]]
    else
      []
    end
  end

  def build_seven_segment_junctions
    junctions = []
    i = 0
    while i < 8
      junctions.push([find_seven_segment_anode_terminal(i), 2, find_led_forward_voltage(0), 15.0])
      i += 1
    end
    junctions
  end

  def add_part(kind, terminal_hole_indices, color_index)
    color_index = count_parts_of_kind(0) % 6 if kind == 0
    part = BreadboardPart.new(kind, terminal_hole_indices, color_index, build_junctions(kind, color_index))
    part.resistance_index = 2
    part.knob_position = 10 / 2
    part.light_level = 10 / 2
    part.transistor_mode = 0
    @parts.push(part)
    update_circuit_structure
  end

  def remove_part(part)
    i = 0
    while i < @parts.length
      if @parts[i] == part
        @parts.delete_at(i)
        break
      end
      i += 1
    end
    update_circuit_structure
  end

  def remove_all_parts
    @parts.clear
    update_circuit_structure
  end

  def update_circuit_structure
    @time_dependent = count_parts_of_kind(15) > 0 || count_parts_of_kind(12) > 0
    @short = false
    assign_nodes
    @circuit_dirty = true
  end

  def operate_part(part)
    case part.kind
    when 6
      part.pressed_frames = 6
    when 7
      part.slide_position = 1 - part.slide_position
    else
      return
    end
    @circuit_dirty = true
  end

  def adjust_part(part, step)
    case part.kind
    when 1
      part.resistance_index = clamp_integer(part.resistance_index + step, 0, 10 - 1)
    when 13
      part.knob_position = clamp_integer(part.knob_position + step, 0, 10)
    when 14
      part.light_level = clamp_integer(part.light_level + step, 0, 10)
    else
      return
    end
    @circuit_dirty = true
  end

  def clamp_integer(value, minimum, maximum)
    return minimum if value < minimum
    return maximum if value > maximum
    value
  end
end
