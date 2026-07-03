class PaintApp
  def init
    @quit = false
    @color_index = 0
    @color_mark = PALETTE_MARK[@color_index]
    @brush = 2
    @pen_down = false
    @cursor_x = CANVAS_W / 2
    @cursor_y = CANVAS_H / 2
    @last_paint_x = -1
    @last_paint_y = -1
    @last_paint_mark = ""
    @pixels = blank_canvas
    @message = "hjkl/yubn move  sp line  p pen  c color  +/- brush"
  end

  def blank_canvas
    pixels = []
    row_text = PALETTE_MARK[ERASER_INDEX] * CANVAS_W
    y = 0
    while y < CANVAS_H
      pixels.push(row_text + "")
      y += 1
    end
    pixels
  end

  def active_color
    PALETTE[@color_index]
  end

  def active_name
    PALETTE_NAME[@color_index]
  end

  def next_color
    restore_cursor_area
    @color_index += 1
    @color_index = 0 if @color_index >= PALETTE.length
    @color_mark = PALETTE_MARK[@color_index]
    @last_paint_x = -1
    @last_paint_mark = ""
    @message = "color: #{active_name}"
    draw_status(@screen)
    draw_cursor(@screen)
    @screen.push(0, 0)
  end

  def bigger_brush
    restore_cursor_area
    @brush += 1
    @brush = 8 if @brush > 8
    @message = "brush: #{@brush}"
    draw_status(@screen)
    draw_cursor(@screen)
    @screen.push(0, 0)
  end

  def smaller_brush
    restore_cursor_area
    @brush -= 1
    @brush = 1 if @brush < 1
    @message = "brush: #{@brush}"
    draw_status(@screen)
    draw_cursor(@screen)
    @screen.push(0, 0)
  end

  def erase_here
    stroke_here(PALETTE_MARK[ERASER_INDEX], "erase")
  end

  def paint_here
    stroke_here(@color_mark, "paint")
  end

  def stroke_here(mark, message)
    restore_cursor_area
    if @last_paint_x < 0 || @last_paint_mark != mark
      draw_dot(mark)
    else
      draw_line(@last_paint_x, @last_paint_y, @cursor_x, @cursor_y, mark)
    end
    @last_paint_x = @cursor_x
    @last_paint_y = @cursor_y
    @last_paint_mark = mark
    @message = message
    draw_status(@screen)
    draw_cursor(@screen)
    @screen.push(0, 0)
  end

  def clear_canvas
    @pixels = blank_canvas
    @last_paint_x = -1
    @last_paint_mark = ""
    @message = "clear"
    draw_screen
  end

  def move_cursor(dx, dy)
    restore_cursor_area
    nx = @cursor_x + dx
    ny = @cursor_y + dy
    nx = 0 if nx < 0
    ny = 0 if ny < 0
    nx = CANVAS_W - 1 if nx >= CANVAS_W
    ny = CANVAS_H - 1 if ny >= CANVAS_H
    @cursor_x = nx
    @cursor_y = ny
    if @pen_down
      if @last_paint_x < 0
        draw_dot(@color_mark)
      else
        draw_line(@last_paint_x, @last_paint_y, @cursor_x, @cursor_y, @color_mark)
      end
      @last_paint_x = @cursor_x
      @last_paint_y = @cursor_y
    end
    draw_cursor(@screen)
    @screen.push(0, 0)
  end

  def toggle_pen
    restore_cursor_area
    @pen_down = !@pen_down
    if @pen_down
      @last_paint_x = @cursor_x
      @last_paint_y = @cursor_y
      draw_dot(@color_mark)
      @message = "pen down"
    else
      @last_paint_x = -1
      @last_paint_mark = ""
      @message = "pen up"
    end
    draw_status(@screen)
    draw_cursor(@screen)
    @screen.push(0, 0)
  end

  def handle(key)
    case key
    when "h"
      move_cursor(-@brush, 0)
    when "l"
      move_cursor(@brush, 0)
    when "k"
      move_cursor(0, -@brush)
    when "j"
      move_cursor(0, @brush)
    when "y"
      move_cursor(-@brush, -@brush)
    when "u"
      move_cursor(@brush, -@brush)
    when "b"
      move_cursor(-@brush, @brush)
    when "n"
      move_cursor(@brush, @brush)
    when " "
      paint_here
    when "p"
      toggle_pen
    when "c"
      next_color
    when "+"
      bigger_brush
    when "="
      bigger_brush
    when "-"
      smaller_brush
    when "e"
      erase_here
    when "x"
      clear_canvas
    when "q"
      @quit = true
    else
      @message = ""
    end
  end
end
