class SlideShow
  def draw
    while @sprite.draw
      render
    end
  end

  def render
    @sprite.fill(Widget.theme_background)

    if @error_message
      Widget.header(@sprite, "SLIDE SHOW")
      Widget.text_center(@sprite, 58, @error_message, Widget.theme_emphasis)
      Widget.footer(@sprite, "ESC exit")
    else
      draw_page
    end
  end

  def draw_page
    page = @pages[@page_index]
    markdown_rows = page[1]
    Widget.header(
      @sprite,
      page[0].split("/")[-1],
      (@page_index + 1).to_s + "/" + @pages.length.to_s
    )

    screen_row_index = 0
    row_index = @top_row_index
    while screen_row_index < VISIBLE_ROW_COUNT && row_index < markdown_rows.length
      screen_y = BODY_TOP + screen_row_index * ROW_HEIGHT
      draw_row(
        markdown_rows[row_index][0],
        markdown_rows[row_index][1],
        screen_y
      )
      screen_row_index += 1
      row_index += 1
    end

    Widget.footer(@sprite, "h/l page  j/k scroll  ESC exit")
  end

  def draw_row(row_kind, row_segments, screen_y)
    screen_x = 4

    case row_kind
    when :heading_1
      @sprite.fill_rect(0, screen_y, Display.width, ROW_HEIGHT, Widget.theme_box)
      draw_segments(row_segments, screen_x, screen_y + 2, row_kind)
    when :heading_2
      draw_segments(row_segments, screen_x + 4, screen_y + 2, row_kind)
    when :heading_3
      draw_segments(row_segments, screen_x + 8, screen_y + 2, row_kind)
    when :list
      @sprite.text(screen_x, screen_y + 2, "-", Widget.theme_emphasis)
      draw_segments(row_segments, screen_x + 12, screen_y + 2, row_kind)
    when :code
      @sprite.fill_rect(0, screen_y, Display.width, ROW_HEIGHT, CODE_BACKGROUND_COLOR)
      draw_segments(row_segments, screen_x, screen_y + 2, row_kind)
    else
      draw_segments(row_segments, screen_x, screen_y + 2, row_kind)
    end
  end

  def draw_segments(row_segments, screen_x, screen_y, row_kind)
    segment_index = 0
    while segment_index < row_segments.length && screen_x < Display.width
      segment_kind = row_segments[segment_index][0]
      segment_text = row_segments[segment_index][1]
      segment_width = Widget.text_width(@sprite, segment_text)

      if segment_kind == :inline_code
        @sprite.fill_rect(
          screen_x,
          screen_y - 1,
          segment_width,
          ROW_HEIGHT - 2,
          CODE_BACKGROUND_COLOR
        )
        @sprite.text(screen_x, screen_y, segment_text, CODE_COLOR)
      else
        case row_kind
        when :heading_1
          @sprite.text(screen_x, screen_y, segment_text, HEADING_1_COLOR)
        when :heading_2
          @sprite.text(screen_x, screen_y, segment_text, HEADING_2_COLOR)
        when :heading_3
          @sprite.text(screen_x, screen_y, segment_text, HEADING_3_COLOR)
        when :code
          @sprite.text(screen_x, screen_y, segment_text, CODE_COLOR)
        else
          @sprite.text(screen_x, screen_y, segment_text, TEXT_COLOR)
        end
      end

      screen_x += segment_width
      segment_index += 1
    end
  end
end
