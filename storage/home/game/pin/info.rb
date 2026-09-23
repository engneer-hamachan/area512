class Breadboard
  def format_header_title
    return format_placement_title if @placing_entry
    hole_index = find_cursor_hole_index
    part = find_part_at(hole_index)
    return format_hole_name(hole_index) if part.nil?
    terminal_index = find_terminal_index(part, hole_index)
    "#{find_part_name(part.kind)} #{find_terminal_name(part.kind, terminal_index)}"
  end

  def format_header_right
    return @message unless @message.empty?
    return "SHORT!" if @short
    return "? help" if @placing_entry
    part = find_part_at(find_cursor_hole_index)
    return "? help" if part.nil?
    format_part_reading(part)
  end

  def find_terminal_index(part, hole_index)
    i = 0
    while i < part.terminal_hole_indices.length
      return i if part.terminal_hole_indices[i] == hole_index
      i += 1
    end
    0
  end

  def format_part_reading(part)
    case part.kind
    when 0
      ""
    when 1
      "#{format_resistance(find_resistor_value(part.resistance_index))} #{format_current(part.current)}"
    when 2, 3, 4
      part.burned ? "BURNED" : format_current(sum_junction_currents(part))
    when 5, 9
      format_current(part.junction_currents[0])
    when 11
      "Ic #{format_current(part.current)}"
    when 8
      "#{format_voltage(5.0)} #{format_current(part.current)}"
    when 15
      format_voltage(part.capacitor_voltage)
    when 12
      part.energized ? "ON" : "OFF"
    when 13
      "knob #{part.knob_position}/#{10}"
    when 14
      "light #{part.light_level}/#{10}"
    when 6
      part.pressed_frames > 0 ? "pressed" : "space"
    when 7
      part.slide_position == 0 ? "A side" : "B side"
    else
      format_current(part.current)
    end
  end

  def sum_junction_currents(part)
    sum = 0.0
    i = 0
    while i < part.junction_currents.length
      sum += part.junction_currents[i]
      i += 1
    end
    sum
  end

  def format_current(amperes)
    microamperes = (amperes.abs * 1000000).to_i
    sign = amperes < 0 ? "-" : ""
    return "#{sign}#{microamperes}uA" if microamperes < 1000
    return "#{sign}#{microamperes / 1000}mA" if microamperes < 1000000
    "#{sign}#{microamperes / 1000000}A"
  end

  def format_voltage(volts)
    tenths = (volts.abs * 10 + 0.5).to_i
    sign = volts < 0 ? "-" : ""
    "#{sign}#{tenths / 10}.#{tenths % 10}V"
  end

  def format_resistance(ohms)
    return "#{ohms}ohm" if ohms < 1000
    return "#{ohms / 1000}k" if ohms % 1000 == 0
    "#{ohms / 1000}.#{ohms % 1000 / 100}k"
  end
end
