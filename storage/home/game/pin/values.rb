class Breadboard
  def find_resistor_value(resistance_index)
    case resistance_index
    when 0 then 10
    when 1 then 100
    when 2 then 220
    when 3 then 330
    when 4 then 470
    when 5 then 1000
    when 6 then 2200
    when 7 then 4700
    when 8 then 10000
    else 100000
    end
  end

  def find_led_forward_voltage(color_index)
    case color_index
    when 0 then 1.8
    when 1 then 2.0
    when 2 then 2.1
    else 3.0
    end
  end

  def find_led_color(color_index)
    case color_index
    when 0 then 0xFF3020
    when 1 then 0xFFD020
    when 2 then 0x30FF40
    else 0x3070FF
    end
  end

  def find_wire_color(color_index)
    case color_index
    when 0 then 0xE03030
    when 1 then 0x3060E0
    when 2 then 0x30A040
    when 3 then 0xE0B020
    when 4 then 0xE07020
    else 0x303030
    end
  end

  def find_rail_name(net)
    case net
    when 0 then "top+"
    when 1 then "top-"
    when 2 then "bottom+"
    else "bottom-"
    end
  end

  def find_seven_segment_anode_terminal(segment_index)
    case segment_index
    when 0 then 3
    when 1 then 4
    when 2 then 8
    when 3 then 6
    when 4 then 5
    when 5 then 1
    when 6 then 0
    else 9
    end
  end
end
