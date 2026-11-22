class Writer
  def wrap_line(line)
    segments = []
    max_width = Display.width - PAGE_PAD * 2 - SCROLLBAR_W
    start_byte = 0
    end_byte = 0
    width = 0
    while end_byte < line.length
      glyph_width = glyph_width_at(line, end_byte)
      if width + glyph_width > max_width
        segments.push(line[start_byte, end_byte - start_byte])
        start_byte = end_byte
        width = 0
      end
      width += glyph_width
      end_byte += utf8_step(line, end_byte)
    end
    segments.push(line[start_byte, end_byte - start_byte])
    segments
  end

  def rebuild_line_segment_counts
    @line_segment_counts = []
    i = 0
    while i < @lines.length
      @line_segment_counts.push(wrap_line(@lines[i]).length)
      i += 1
    end
  end

  def locate_cursor
    segments = wrap_line(current_line)
    last_segment_index = segments.length - 1
    @cursor_segment_index = 0
    @cursor_segment_col = @col
    while @cursor_segment_index < last_segment_index
      segment_char_count = char_count(segments[@cursor_segment_index])
      break if @cursor_segment_col < segment_char_count
      @cursor_segment_col -= segment_char_count
      @cursor_segment_index += 1
    end
  end

  def scroll_down_one_segment
    @top_segment_index += 1
    if @top_segment_index >= @line_segment_counts[@top]
      @top += 1
      @top_segment_index = 0
    end
  end

  def cursor_above_page?
    return true if @row < @top
    @row == @top && @cursor_segment_index < @top_segment_index
  end

  def adjust_scroll
    if cursor_above_page?
      @top = @row
      @top_segment_index = @cursor_segment_index
    end
    last_top_segment_index = @line_segment_counts[@top] - 1
    if @top_segment_index > last_top_segment_index
      @top_segment_index = last_top_segment_index
    end
    rows = visible_rows
    segments_through_cursor = @cursor_segment_index + 1 - @top_segment_index
    r = @top
    while r < @row
      segments_through_cursor += @line_segment_counts[r]
      r += 1
    end
    while segments_through_cursor > rows
      scroll_down_one_segment
      segments_through_cursor -= 1
    end
  end

  def move_cursor_to_segment(row, segment_index, segment_col)
    segments = wrap_line(@lines[row])
    segment_start_col = 0
    i = 0
    while i < segment_index
      segment_start_col += char_count(segments[i])
      i += 1
    end
    max_segment_col = char_count(segments[segment_index])
    max_segment_col -= 1 if segment_index < segments.length - 1
    segment_col = max_segment_col if segment_col > max_segment_col
    @row = row
    @col = segment_start_col + segment_col
    clamp_cursor
  end

  def move_up
    if @cursor_segment_index > 0
      target_segment_index = @cursor_segment_index - 1
      move_cursor_to_segment(@row, target_segment_index, @cursor_segment_col)
    elsif @row > 0
      last_segment_index = @line_segment_counts[@row - 1] - 1
      move_cursor_to_segment(@row - 1, last_segment_index, @cursor_segment_col)
    end
  end

  def move_down
    if @cursor_segment_index < @line_segment_counts[@row] - 1
      target_segment_index = @cursor_segment_index + 1
      move_cursor_to_segment(@row, target_segment_index, @cursor_segment_col)
    elsif @row < @lines.length - 1
      move_cursor_to_segment(@row + 1, 0, @cursor_segment_col)
    end
  end

  def page_up
    i = 0
    while i < visible_rows
      move_up
      i += 1
    end
    @msg = "Scroll"
  end

  def page_down
    i = 0
    while i < visible_rows
      move_down
      i += 1
    end
    @msg = "Scroll"
  end
end
