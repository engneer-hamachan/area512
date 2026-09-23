class Breadboard
  def stamp_part(part)
    node_indices = part.terminal_node_indices
    case part.kind
    when 1
      stamp_conductance(node_indices[0], node_indices[1], 1.0 / find_resistor_value(part.resistance_index))
    when 8
      stamp_source(node_indices[0], node_indices[1], 5.0, 0.5)
    when 10
      stamp_conductance(node_indices[0], node_indices[1], 1.0 / 25.0)
    when 14
      stamp_conductance(node_indices[0], node_indices[1], 1.0 / compute_cds_resistance(part))
    when 15
      stamp_source(node_indices[0], node_indices[1], part.capacitor_voltage, compute_capacitor_resistance)
    when 13
      stamp_conductance(node_indices[0], node_indices[1], 1.0 / compute_wiper_resistance(part.knob_position))
      stamp_conductance(node_indices[1], node_indices[2], 1.0 / compute_wiper_resistance(10 - part.knob_position))
    when 6
      stamp_contact(node_indices[0], node_indices[1]) if part.pressed_frames > 0
    when 7
      stamp_contact(node_indices[1], node_indices[part.slide_position * 2])
    when 11
      stamp_transistor_collector(part)
    when 12
      stamp_conductance(node_indices[0], node_indices[2], 1.0 / 100.0)
      stamp_contact(node_indices[1], node_indices[part.energized ? 4 : 3])
    end
    stamp_junctions(part)
  end

  def compute_cds_resistance(part)
    200000.0 / (2 ** part.light_level)
  end

  def compute_capacitor_resistance
    0.05 / 0.001
  end

  def compute_wiper_resistance(knob_distance)
    10000.0 * knob_distance / 10 + 0.01
  end

  def compute_part_current(part)
    case part.kind
    when 1
      part.voltage / find_resistor_value(part.resistance_index)
    when 8
      (5.0 - part.voltage) / 0.5
    when 10
      part.voltage / 25.0
    when 14
      part.voltage / compute_cds_resistance(part)
    when 15
      (part.voltage - part.capacitor_voltage) / compute_capacitor_resistance
    when 12
      (read_terminal_voltage(part, 0) - read_terminal_voltage(part, 2)) / 100.0
    else
      0.0
    end
  end
end
