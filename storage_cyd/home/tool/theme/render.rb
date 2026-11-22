class ThemeEditor
  def draw
    while @sprite.draw
      if @preview
        render_preview
      else
        render_list
      end
    end
  end

  def render_list
    @sprite.fill(Widget.theme_background)
    Widget.header(@sprite, "THEME", @modified ? "modified" : "")

    end_row_index = [@top_row_index + visible_row_count, ROW_COUNT].min
    row_index = @top_row_index
    while row_index < end_row_index
      draw_row(row_index)
      row_index += 1
    end
    Widget.scrollbar(
      @sprite,
      Display.width - SCROLLBAR_WIDTH,
      Widget.body_top,
      visible_row_count * ROW_HEIGHT,
      @top_row_index,
      visible_row_count,
      ROW_COUNT
    )

    Widget.toast(@sprite, @message) if @message
    Widget.footer(@sprite, "hl ch +-[] val ENTER TAB s ESC")
  end

  def visible_row_count
    Widget.body_height / ROW_HEIGHT
  end

  def row_top(row_index)
    Widget.body_top + (row_index - @top_row_index) * ROW_HEIGHT
  end

  def draw_row(row_index)
    if row_index == IMAGE_ROW_INDEX
      draw_image_row
    elsif row_index == CRT_ROW_INDEX
      draw_crt_row
    else
      draw_color_row(row_index)
    end
  end

  def draw_row_label(row_index, label)
    selected = row_index == @row_index
    label_color = selected ? Widget.theme_selected : Widget.theme_text
    if selected
      @sprite.fill_rect(0, row_top(row_index), Display.width, ROW_HEIGHT, Widget.theme_box)
    end
    @sprite.text(LABEL_X, row_top(row_index) + 1, label, label_color)
  end

  def draw_color_row(color_index)
    y = row_top(color_index) + 1
    color = @colors[color_index]
    selected = color_index == @row_index

    draw_row_label(color_index, COLOR_KEYS[color_index])
    @sprite.fill_rect(SWATCH_X, y, SWATCH_WIDTH, ROW_HEIGHT - 2, color)
    @sprite.rect(SWATCH_X, y, SWATCH_WIDTH, ROW_HEIGHT - 2, Widget.theme_border)

    @sprite.text(HEX_X, y, "0x", Widget.theme_text)
    channel_index = 0
    while channel_index < CHANNEL_SHIFTS.length
      channel_color = selected && channel_index == @channel_index ? Widget.theme_emphasis : Widget.theme_text
      @sprite.text(
        HEX_X + (2 + channel_index * 2) * Widget.char_width,
        y,
        format_hex_byte(read_channel(color, channel_index)),
        channel_color
      )
      channel_index += 1
    end

    return unless selected

    Widget.text_right(
      @sprite,
      Display.width - LABEL_X,
      y,
      CHANNEL_NAMES[@channel_index] + " " + read_channel(color, @channel_index).to_s,
      Widget.theme_emphasis
    )
  end

  def draw_image_row
    draw_row_label(IMAGE_ROW_INDEX, "image")
    @sprite.text(
      SWATCH_X,
      row_top(IMAGE_ROW_INDEX) + 1,
      Widget.clip(@sprite, image_label, Display.width - SWATCH_X - LABEL_X),
      Widget.theme_text
    )
  end

  def draw_crt_row
    draw_row_label(CRT_ROW_INDEX, "crt")
    @sprite.text(
      SWATCH_X,
      row_top(CRT_ROW_INDEX) + 1,
      @crt_enabled ? "on" : "off",
      Widget.theme_text
    )
  end

  def image_label
    return "(none)" if @background_image == ""

    @background_image.split("/")[-1]
  end
end
