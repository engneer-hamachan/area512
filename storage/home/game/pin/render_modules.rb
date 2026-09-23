class Breadboard
  def compute_terminal_bounds(part)
    left = compute_hole_x(part.terminal_hole_indices[0])
    top = compute_hole_y(part.terminal_hole_indices[0])
    right = left
    bottom = top
    i = 1
    while i < part.terminal_hole_indices.length
      x = compute_hole_x(part.terminal_hole_indices[i])
      y = compute_hole_y(part.terminal_hole_indices[i])
      left = x if x < left
      right = x if x > right
      top = y if y < top
      bottom = y if y > bottom
      i += 1
    end
    [left - 3, top - 3, right + 3, bottom + 3]
  end

  def draw_module_part(sprite, part)
    bounds = compute_terminal_bounds(part)
    left = bounds[0]
    top = bounds[1]
    right = bounds[2]
    width = right - left + 1
    height = bounds[3] - top + 1
    center_x = left + width / 2
    center_y = top + height / 2
    case part.kind
    when 3
      sprite.fill_circle(center_x, center_y, 5, compute_rgb_color(part))
    when 4
      sprite.fill_rect(left, top, width, height, 0x202020)
      draw_seven_segments(sprite, part, center_x, center_y)
    when 6
      sprite.fill_rect(left, top, width, height, 0x202020)
      button_color = part.pressed_frames > 0 ? 0xFFD020 : 0xB0B0B0
      sprite.fill_circle(center_x, center_y, 3, button_color)
    when 7
      sprite.fill_rect(left, top, width, height, 0x2050B0)
      knob_x = part.slide_position == 0 ? left + 1 : center_x
      sprite.fill_rect(knob_x, top + 1, width / 2, height - 2, 0xB0B0B0)
    when 11
      sprite.fill_rect(left, top, width, height, 0x202020)
      sprite.line(left, top, right, top, 0xB0B0B0)
    when 13
      sprite.fill_rect(left, top, width, height, 0x2050B0)
      draw_knob(sprite, part, center_x, center_y)
    when 12
      sprite.fill_rect(left, top, width, height, 0x2050B0)
      contact_color = part.energized ? 0xFFD020 : 0xB0B0B0
      sprite.fill_rect(center_x - 3, center_y - 3, 7, 7, contact_color)
    end
    draw_module_pins(sprite, part)
  end

  def draw_module_pins(sprite, part)
    i = 0
    while i < part.terminal_hole_indices.length
      sprite.pixel(compute_hole_x(part.terminal_hole_indices[i]), compute_hole_y(part.terminal_hole_indices[i]), 0xB0B0B0)
      i += 1
    end
  end

  def compute_rgb_color(part)
    return 0x282828 if part.burned
    return 0xB0B0B0 if part.levels.length < 3 || part.levels.max == 0
    red = 255 * part.levels[0] / 8
    green = 255 * part.levels[1] / 8
    blue = 255 * part.levels[2] / 8
    (red << 16) | (green << 8) | blue
  end

  def draw_knob(sprite, part, center_x, center_y)
    sprite.fill_circle(center_x, center_y, 3, 0xB0B0B0)
    tip_x = center_x - 3 + part.knob_position * 6 / 10
    sprite.line(center_x, center_y, tip_x, center_y - 3, 0x202020)
  end

  def draw_seven_segments(sprite, part, center_x, center_y)
    i = 0
    while i < 8
      level = (part.burned || part.levels.length <= i) ? 0 : part.levels[i]
      draw_seven_segment(sprite, i, center_x, center_y, blend_color(0x401010, 0xFF3020, level))
      i += 1
    end
  end

  def draw_seven_segment(sprite, segment_index, center_x, center_y, color)
    case segment_index
    when 0 then sprite.fill_rect(center_x - 4, center_y - 10, 9, 2, color)
    when 1 then sprite.fill_rect(center_x + 5, center_y - 9, 2, 8, color)
    when 2 then sprite.fill_rect(center_x + 5, center_y + 1, 2, 8, color)
    when 3 then sprite.fill_rect(center_x - 4, center_y + 9, 9, 2, color)
    when 4 then sprite.fill_rect(center_x - 6, center_y + 1, 2, 8, color)
    when 5 then sprite.fill_rect(center_x - 6, center_y - 9, 2, 8, color)
    when 6 then sprite.fill_rect(center_x - 4, center_y - 1, 9, 2, color)
    else sprite.fill_rect(center_x + 8, center_y + 9, 2, 2, color)
    end
  end
end
