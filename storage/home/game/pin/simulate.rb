class Breadboard
  def step_simulation
    count_down_tact_switches
    solve_circuit if @circuit_dirty || @time_dependent
    advance_motors
  end

  def count_down_tact_switches
    i = 0
    while i < @parts.length
      part = @parts[i]
      if part.pressed_frames > 0
        part.pressed_frames -= 1
        if part.pressed_frames == 0
          @circuit_dirty = true
          @needs_redraw = true
        end
      end
      i += 1
    end
  end

  def solve_circuit
    @circuit_dirty = false
    return if @node_count == 0
    iteration = 0
    while iteration < 12
      clear_matrix
      i = 0
      while i < @parts.length
        stamp_part(@parts[i])
        i += 1
      end
      solve_linear_system
      break unless update_semiconductor_states
      iteration += 1
    end
    update_parts_from_solution
  end

  def update_parts_from_solution
    @short = false
    i = 0
    while i < @parts.length
      update_part_from_solution(@parts[i])
      i += 1
    end
  end

  def update_part_from_solution(part)
    part.voltage = read_terminal_voltage(part, 0) - read_terminal_voltage(part, 1)
    part.current = compute_part_current(part)
    part.current = compute_collector_current(part) if part.kind == 11
    record_junction_currents(part)
    burn_overloaded_led(part)
    case part.kind
    when 8
      @short = true if part.current > 2.0
    when 15
      part.capacitor_voltage = part.voltage
    when 12
      energized = part.current.abs > 0.03
      @circuit_dirty = true if energized != part.energized
      part.energized = energized
    end
    levels = compute_part_levels(part)
    @needs_redraw = true if levels != part.levels
    part.levels = levels
  end

  def record_junction_currents(part)
    i = 0
    while i < part.junctions.length
      part.junction_currents[i] = compute_junction_current(part, i)
      i += 1
    end
  end

  def burn_overloaded_led(part)
    return unless part.kind == 2 || part.kind == 3 || part.kind == 4
    return unless part.junction_currents.max > 0.05
    part.burned = true
    part.junction_states = Array.new(part.junctions.length, false)
    @circuit_dirty = true
  end

  def compute_part_levels(part)
    case part.kind
    when 2, 3, 4
      compute_brightness_levels(part)
    when 9
      [part.junction_currents[0] > 0.005 ? 1 : 0]
    when 15
      [clamp_integer((part.capacitor_voltage.abs / 5.0 * 8).to_i, 0, 8)]
    when 12
      [part.energized ? 1 : 0]
    else
      []
    end
  end

  def compute_brightness_levels(part)
    levels = []
    i = 0
    while i < part.junction_currents.length
      levels.push(compute_brightness_level(part.junction_currents[i]))
      i += 1
    end
    levels
  end

  def compute_brightness_level(current)
    return 0 if current < 0.0005
    level = (current / 0.02 * 8).to_i
    clamp_integer(level, 1, 8)
  end

  def compute_motor_phase_step(part)
    clamp_integer((part.current * 20.0).to_i, -16 / 4, 16 / 4)
  end

  def advance_motors
    i = 0
    while i < @parts.length
      part = @parts[i]
      if part.kind == 10
        part.motor_phase = (part.motor_phase + compute_motor_phase_step(part) + 16) % 16
      end
      i += 1
    end
  end

  def animation_running?
    i = 0
    while i < @parts.length
      part = @parts[i]
      return true if part.kind == 10 && compute_motor_phase_step(part) != 0
      return true if part.kind == 9 && part.levels[0] == 1
      i += 1
    end
    false
  end
end
