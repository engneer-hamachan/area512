class Breadboard
  def draw_parts(sprite)
    i = 0
    while i < @parts.length
      part = @parts[i]
      if has_footprint?(part.kind)
        draw_module_part(sprite, part)
      else
        draw_lead_part(sprite, part)
      end
      i += 1
    end
  end

  def draw_lead_part(sprite, part)
    start_x = compute_hole_x(part.terminal_hole_indices[0])
    start_y = compute_hole_y(part.terminal_hole_indices[0])
    end_x = compute_hole_x(part.terminal_hole_indices[1])
    end_y = compute_hole_y(part.terminal_hole_indices[1])
    middle_x = (start_x + end_x) / 2
    middle_y = (start_y + end_y) / 2
    if part.kind == 0
      draw_wire(sprite, start_x, start_y, end_x, end_y, find_wire_color(part.color_index))
      return
    end
    sprite.line(start_x, start_y, end_x, end_y, 0x8A8A8A)
    case part.kind
    when 1
      sprite.fill_rect(middle_x - 4, middle_y - 2, 9, 5, 0xD8B070)
      sprite.fill_rect(middle_x - 2, middle_y - 2, 1, 5, 0x803010)
      sprite.fill_rect(middle_x + 1, middle_y - 2, 1, 5, 0x803010)
    when 2
      draw_led(sprite, part, middle_x, middle_y)
    when 5
      sprite.fill_rect(middle_x - 3, middle_y - 2, 7, 5, 0x202020)
      draw_terminal_band(sprite, middle_x, middle_y, end_x, end_y)
    when 8
      sprite.fill_rect(middle_x - 6, middle_y - 3, 13, 7, 0x202020)
      draw_terminal_band(sprite, middle_x, middle_y, start_x, start_y)
    when 9
      draw_buzzer(sprite, part, middle_x, middle_y)
    when 10
      draw_motor(sprite, part, middle_x, middle_y)
    when 14
      sprite.fill_circle(middle_x, middle_y, 3, 0xD08040)
      sprite.line(middle_x - 2, middle_y, middle_x + 2, middle_y, 0x803010)
    when 15
      draw_capacitor(sprite, part, middle_x, middle_y)
    end
  end

  def draw_wire(sprite, start_x, start_y, end_x, end_y, color)
    sprite.line(start_x, start_y, end_x, end_y, color)
    sprite.line(start_x + 1, start_y, end_x + 1, end_y, color)
    sprite.line(start_x, start_y + 1, end_x, end_y + 1, color)
    sprite.fill_rect(start_x - 1, start_y - 1, 3, 3, color)
    sprite.fill_rect(end_x - 1, end_y - 1, 3, 3, color)
  end

  def draw_terminal_band(sprite, middle_x, middle_y, toward_x, toward_y)
    band_x = middle_x + compute_sign(toward_x - middle_x) * 2
    band_y = middle_y + compute_sign(toward_y - middle_y) * 2
    sprite.fill_rect(band_x - 1, band_y - 1, 3, 3, 0xB0B0B0)
  end

  def compute_sign(value)
    return 1 if value > 0
    return -1 if value < 0
    0
  end

  def draw_led(sprite, part, center_x, center_y)
    bright_color = find_led_color(part.color_index)
    if part.burned
      sprite.fill_circle(center_x, center_y, 3, 0x282828)
      sprite.line(center_x - 2, center_y - 2, center_x + 2, center_y + 2, bright_color)
      sprite.line(center_x - 2, center_y + 2, center_x + 2, center_y - 2, bright_color)
      return
    end
    unlit_color = blend_color(0x282828, bright_color, 8 / 4)
    level = part.levels.empty? ? 0 : part.levels[0]
    sprite.fill_circle(center_x, center_y, 3, blend_color(unlit_color, bright_color, level))
    sprite.circle(center_x, center_y, 5, bright_color) if level == 8
  end

  def draw_buzzer(sprite, part, center_x, center_y)
    sprite.fill_circle(center_x, center_y, 4, 0x202020)
    return if part.levels.empty? || part.levels[0] == 0
    sprite.circle(center_x, center_y, 6 + @frame_count % 3, 0xFFD020)
  end

  def draw_motor(sprite, part, center_x, center_y)
    sprite.fill_circle(center_x, center_y, 5, 0xB0B0B0)
    angle = part.motor_phase * 0.39269908169872414
    tip_x = center_x + (Math.cos(angle) * 4).to_i
    tip_y = center_y + (Math.sin(angle) * 4).to_i
    sprite.line(center_x, center_y, tip_x, tip_y, 0x202020)
  end

  def draw_capacitor(sprite, part, center_x, center_y)
    sprite.fill_circle(center_x, center_y, 4, 0x2050B0)
    return if part.levels.empty? || part.levels[0] == 0
    charge_height = part.levels[0] * 6 / 8
    sprite.fill_rect(center_x - 1, center_y + 3 - charge_height, 3, charge_height, 0xFFD020)
  end
end
