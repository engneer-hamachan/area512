class Breadboard
  def find_part_name(kind)
    case kind
    when 0 then "Wire"
    when 1 then "Resistor"
    when 2 then "LED"
    when 3 then "RGB LED"
    when 4 then "7-seg"
    when 5 then "Diode"
    when 6 then "Tact SW"
    when 7 then "Slide SW"
    when 8 then "Battery"
    when 9 then "Buzzer"
    when 10 then "Motor"
    when 11 then "NPN Tr"
    when 12 then "Relay"
    when 13 then "Volume"
    when 14 then "CdS"
    else "Capacitor"
    end
  end

  def find_terminal_name(kind, terminal_index)
    case kind
    when 0
      "end"
    when 2, 5
      terminal_index == 0 ? "anode+" : "cathode-"
    when 8, 9, 15
      terminal_index == 0 ? "+" : "-"
    when 3
      find_rgb_terminal_name(terminal_index)
    when 4
      find_seven_segment_terminal_name(terminal_index)
    when 6
      terminal_index % 2 == 0 ? "left" : "right"
    when 7
      terminal_index == 1 ? "common" : (terminal_index == 0 ? "A" : "B")
    when 13
      terminal_index == 1 ? "wiper" : (terminal_index == 0 ? "A" : "B")
    when 11
      find_transistor_terminal_name(terminal_index)
    when 12
      find_relay_terminal_name(terminal_index)
    else
      "lead"
    end
  end

  def find_rgb_terminal_name(terminal_index)
    case terminal_index
    when 0 then "red"
    when 1 then "common-"
    when 2 then "green"
    else "blue"
    end
  end

  def find_seven_segment_terminal_name(terminal_index)
    case terminal_index
    when 0 then "g"
    when 1 then "f"
    when 3 then "a"
    when 4 then "b"
    when 5 then "e"
    when 6 then "d"
    when 8 then "c"
    when 9 then "dp"
    else "common-"
    end
  end

  def find_transistor_terminal_name(terminal_index)
    case terminal_index
    when 0 then "emitter"
    when 1 then "base"
    else "collector"
    end
  end

  def find_relay_terminal_name(terminal_index)
    case terminal_index
    when 1 then "common"
    when 3 then "NC"
    when 4 then "NO"
    else "coil"
    end
  end
end
