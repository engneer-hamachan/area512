class Breadboard
  def draw_screen
    while @sprite.draw
      draw_band
    end
  end

  def draw_band
    sprite = @sprite
    sprite.fill(Widget.theme_background)
    draw_board(sprite)
    draw_parts(sprite)
    draw_placement_preview(sprite) if @placing_entry
    draw_cursor(sprite)
    Widget.header(sprite, format_header_title, format_header_right)
    draw_help(sprite) if @help_visible
  end

  def draw_board(sprite)
    top_y = compute_row_y(0) - 3 - 1
    bottom_y = compute_row_y(14 - 1) + 3 + 2
    sprite.fill_rect(12, top_y, 216, bottom_y - top_y, 0xE8E2D0)
    groove_y = (compute_row_y(7 - 1) + compute_row_y(7)) / 2
    sprite.fill_rect(12, groove_y - 1, 216, 3, 0xC4BCAA)
    draw_rail_line(sprite, compute_row_y(0) - 3, 0xD03030)
    draw_rail_line(sprite, compute_row_y(2 - 1) + 3, 0x3050D0)
    draw_rail_line(sprite, compute_row_y(12) - 3, 0xD03030)
    draw_rail_line(sprite, compute_row_y(14 - 1) + 3, 0x3050D0)
    draw_holes(sprite)
  end

  def draw_rail_line(sprite, y, color)
    sprite.line(12 + 2, y, 12 + 216 - 3, y, color)
  end

  def draw_holes(sprite)
    row = 0
    while row < 14
      y = compute_row_y(row)
      if y + 1 >= sprite.region_top && y - 1 < sprite.region_bottom
        column = 0
        while column < 30
          sprite.fill_rect(compute_column_x(column) - 1, y - 1, 3, 3, 0x4A4A4A)
          column += 1
        end
      end
      row += 1
    end
  end

  def draw_hole_marker(sprite, hole_index, color)
    sprite.rect(compute_hole_x(hole_index) - 3, compute_hole_y(hole_index) - 3, 7, 7, color)
  end

  def draw_cursor(sprite)
    draw_hole_marker(sprite, find_cursor_hole_index, 0x00C0FF)
  end

  def draw_placement_preview(sprite)
    hole_indices = compute_placement_hole_indices
    return if hole_indices.nil?
    color = placement_allowed? ? 0x20C020 : 0xFF0000
    if hole_indices.length == 2 && !has_footprint?(read_placing_kind)
      sprite.line(compute_hole_x(hole_indices[0]), compute_hole_y(hole_indices[0]),
                  compute_hole_x(hole_indices[1]), compute_hole_y(hole_indices[1]), color)
    end
    i = 0
    while i < hole_indices.length
      draw_hole_marker(sprite, hole_indices[i], color)
      i += 1
    end
  end

  def draw_help(sprite)
    top_y = Widget.header_height + 2
    Widget.panel(sprite, 8, top_y, Display.width - 16, 8 * 12 + 8)
    i = 0
    while i < 8
      sprite.text(14, top_y + 4 + i * 12, find_help_line(i), Widget.theme_text)
      i += 1
    end
  end

  def find_help_line(line_index)
    case line_index
    when 0 then "hjkl / arrows  move"
    when 1 then "a / Enter  add part"
    when 2 then "space  press or flip switch"
    when 3 then "+ -  change value / knob"
    when 4 then "x / BS  remove part"
    when 5 then "c  clear board"
    when 6 then "q  quit"
    else "any key  close help"
    end
  end

  def blend_color(dark_color, bright_color, level)
    red = blend_channel(dark_color >> 16, bright_color >> 16, level)
    green = blend_channel(dark_color >> 8, bright_color >> 8, level)
    blue = blend_channel(dark_color, bright_color, level)
    (red << 16) | (green << 8) | blue
  end

  def blend_channel(dark_value, bright_value, level)
    dark_channel = dark_value & 0xFF
    bright_channel = bright_value & 0xFF
    dark_channel + (bright_channel - dark_channel) * level / 8
  end
end
