class PaintApp
  def draw_dot(mark)
    draw_dot_at(@cursor_x, @cursor_y, mark)
  end

  def draw_dot_at(cx, cy, mark)
    r = @brush - 1
    r = 0 if r < 0
    y = cy - r
    while y <= cy + r
      x = cx - r
      while x <= cx + r
        if x >= 0 && x < CANVAS_W && y >= 0 && y < CANVAS_H
          dx = x - cx
          dy = y - cy
          if dx * dx + dy * dy <= r * r
            row = @pixels.at(y)
            if row.is_a?(String)
              row[x] = mark
              draw_cell(@screen, x, y)
            end
          end
        end
        x += 1
      end
      y += 1
    end
  end

  def draw_line(x0, y0, x1, y1, mark)
    dx = x1 - x0
    dy = y1 - y0
    steps = absi(dx)
    ay = absi(dy)
    steps = ay if ay > steps
    if steps == 0
      draw_dot_at(x1, y1, mark)
      return
    end
    i = 0
    while i <= steps
      x = x0 + dx * i / steps
      y = y0 + dy * i / steps
      draw_dot_at(x, y, mark)
      i += 1
    end
  end

  def status_text
    mode = @pen_down ? "pen" : "move"
    "#{mode} #{active_name} b#{@brush}"
  end

  def draw_status(sp)
    sp.fill_rect(0, 0, APP_W, STATUS_H, BG_COLOR)
    sp.text(0, 2, status_text, TEXT_COLOR)
    x = 150
    i = 0
    while i < PALETTE.length
      sp.fill_rect(x, 3, 6, 6, PALETTE[i])
      sp.rect(x, 3, 6, 6, CURSOR_COLOR) if i == @color_index
      x += 8
      i += 1
    end
  end

  def draw_cursor(sp)
    sx = @cursor_x * CELL
    sy = CANVAS_Y + @cursor_y * CELL
    r = @brush * CELL + 2
    sp.circle(sx, sy, r, CURSOR_COLOR)
    sp.pixel(sx, sy, CURSOR_COLOR)
  end

  def draw_cell(sp, x, y)
    row = @pixels.at(y)
    if row.is_a?(String)
      byte = row.getbyte(x)
      if byte.is_a?(Integer)
        color = PALETTE[byte - 48]
        sp.fill_rect(x * CELL, CANVAS_Y + y * CELL, CELL, CELL, color)
      end
    end
  end

  def restore_cursor_area
    r = @brush + 2
    y = @cursor_y - r
    while y <= @cursor_y + r
      x = @cursor_x - r
      while x <= @cursor_x + r
        draw_cell(@screen, x, y) if x >= 0 && x < CANVAS_W && y >= 0 && y < CANVAS_H
        x += 1
      end
      y += 1
    end
  end

  def draw_canvas(sp)
    y = 0
    while y < CANVAS_H
      x = 0
      while x < CANVAS_W
        draw_cell(sp, x, y)
        x += 1
      end
      y += 1
    end
  end

  def draw_screen
    @screen.fill(BG_COLOR)
    draw_status(@screen)
    draw_canvas(@screen)
    draw_cursor(@screen)
    @screen.push(0, 0)
  end

  def draw_goodbye
    @screen.fill(BG_COLOR)
    @screen.text(0, 10, "Paint closed.", TEXT_COLOR)
    @screen.text(0, 30, "Press any key.", 0x888888)
    @screen.push(0, 0)
  end
end
