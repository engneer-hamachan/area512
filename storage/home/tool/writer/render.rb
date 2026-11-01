class Writer
  def draw
    layout_page
    count_scrollbar_segments
    while @sp.draw
      render
    end
  end

  def layout_page
    @page_segments = []
    rows = visible_rows
    r = @top
    while r < @lines.length && @page_segments.length < rows
      segments = wrap_line(@lines[r])
      i = r == @top ? @top_segment_index : 0
      while i < segments.length && @page_segments.length < rows
        if r == @row && i == @cursor_segment_index
          @cursor_page_segment_index = @page_segments.length
        end
        @page_segments.push(segments[i])
        i += 1
      end
      r += 1
    end
  end

  def count_scrollbar_segments
    @segments_above_page = @top_segment_index
    @total_segment_count = 0
    i = 0
    while i < @line_segment_counts.length
      @segments_above_page += @line_segment_counts[i] if i < @top
      @total_segment_count += @line_segment_counts[i]
      i += 1
    end
  end

  def render
    @sp.fill(Widget.theme_background)
    Widget.header(@sp, "WRITER", doc_label + dirty_mark)
    draw_page
    draw_status
  end

  def draw_page
    y = Widget.body_top
    w = Display.width
    h = Widget.body_height
    Widget.panel(@sp, 0, y, w, h)
    draw_segments(PAGE_PAD, y + PAGE_PAD)
    Widget.scrollbar(
      @sp,
      w - PAGE_PAD - SCROLLBAR_W,
      y + PAGE_PAD,
      h - PAGE_PAD * 2,
      @segments_above_page,
      visible_rows,
      @total_segment_count
    )
  end

  def draw_segments(x, y)
    yy = y
    i = 0
    while i < @page_segments.length
      @sp.text(x, yy, @page_segments[i], Widget.theme_text)
      draw_cursor(x, yy, @page_segments[i]) if i == @cursor_page_segment_index
      yy += LINE_H
      i += 1
    end
  end

  def draw_cursor(x, y, segment)
    cx = x + text_width(segment, @cursor_segment_col)
    @sp.line(cx, y - 1, cx, y + LINE_H - 2, Widget.theme_selected)
  end

  def draw_status
    pos = (@row + 1).to_s + ":" + (@col + 1).to_s
    Widget.footer(@sp, pos + " " + @msg)
  end
end
