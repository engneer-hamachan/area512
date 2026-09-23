class Breadboard
  def stamp_junctions(part)
    return if part.burned
    node_indices = part.terminal_node_indices
    junctions = part.junctions
    i = 0
    while i < junctions.length
      junction = junctions[i]
      stamp_source(node_indices[junction[0]], node_indices[junction[1]], junction[2], junction[3]) if part.junction_states[i]
      i += 1
    end
  end

  def compute_junction_current(part, junction_index)
    return 0.0 unless part.junction_states[junction_index]
    junction = part.junctions[junction_index]
    voltage_across = read_terminal_voltage(part, junction[0]) - read_terminal_voltage(part, junction[1])
    (voltage_across - junction[2]) / junction[3]
  end

  def update_junction_states(part)
    return false if part.burned
    changed = false
    i = 0
    while i < part.junctions.length
      junction = part.junctions[i]
      if part.junction_states[i]
        conducting = compute_junction_current(part, i) >= 0.0
      else
        conducting = read_terminal_voltage(part, junction[0]) - read_terminal_voltage(part, junction[1]) > junction[2]
      end
      if conducting != part.junction_states[i]
        part.junction_states[i] = conducting
        changed = true
      end
      i += 1
    end
    changed
  end

  def stamp_transistor_collector(part)
    node_indices = part.terminal_node_indices
    case part.transistor_mode
    when 1
      gain_conductance = 100.0 / 10.0
      stamp_controlled_current(node_indices[2], node_indices[0], node_indices[1], gain_conductance, 0.7)
    when 2
      stamp_source(node_indices[2], node_indices[0], 0.2, 1.0)
    end
  end

  def compute_collector_current(part)
    case part.transistor_mode
    when 1
      100.0 * compute_junction_current(part, 0)
    when 2
      collector_emitter_voltage = read_terminal_voltage(part, 2) - read_terminal_voltage(part, 0)
      (collector_emitter_voltage - 0.2) / 1.0
    else
      0.0
    end
  end

  def choose_transistor_mode(part)
    return 0 unless part.junction_states[0]
    collector_emitter_voltage = read_terminal_voltage(part, 2) - read_terminal_voltage(part, 0)
    case part.transistor_mode
    when 1
      return 2 if collector_emitter_voltage < 0.2
    when 2
      return 1 if compute_collector_current(part) > 100.0 * compute_junction_current(part, 0)
      return 2
    end
    1
  end

  def update_semiconductor_states
    changed = false
    i = 0
    while i < @parts.length
      part = @parts[i]
      changed = true if update_junction_states(part)
      if part.kind == 11
        mode = choose_transistor_mode(part)
        if mode != part.transistor_mode
          part.transistor_mode = mode
          changed = true
        end
      end
      i += 1
    end
    changed
  end
end
