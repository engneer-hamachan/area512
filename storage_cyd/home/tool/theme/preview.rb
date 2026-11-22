class ThemeEditor
  def render_preview
    @sprite.fill(@colors[BACKGROUND_INDEX])
    draw_preview_header
    draw_preview_body
    draw_preview_panel
    draw_preview_footer
  end

  def draw_preview_header
    header_height = Widget.header_height
    @sprite.fill_rect(0, 0, Display.width, header_height, @colors[BOX_INDEX])
    @sprite.line(0, header_height - 1, Display.width - 1, header_height - 1, @colors[BORDER_INDEX])
    @sprite.text(LABEL_X, 2, "PREVIEW", @colors[SELECTED_INDEX])
    Widget.text_right(@sprite, Display.width - LABEL_X, 2, "TAB back", @colors[TEXT_INDEX])
  end

  def draw_preview_body
    y = Widget.body_top
    @sprite.text(LABEL_X, y, "Body text", @colors[TEXT_INDEX])
    @sprite.text(Display.width / 2, y, "Emphasis", @colors[EMPHASIS_INDEX])
  end

  def draw_preview_panel
    x = LABEL_X
    y = Widget.body_top + 18
    width = Display.width - LABEL_X * 2
    height = ROW_HEIGHT * 3

    @sprite.fill_rect(x, y, width, height, @colors[BOX_INDEX])
    @sprite.rect(x, y, width, height, @colors[BORDER_INDEX])
    @sprite.rect(x + 4, y + 4, width - 8, ROW_HEIGHT + 2, @colors[SELECTED_INDEX])
    @sprite.text(x + 8, y + 6, "Selected item", @colors[SELECTED_INDEX])
    @sprite.text(x + 8, y + 6 + ROW_HEIGHT + 4, "Normal item", @colors[TEXT_INDEX])
  end

  def draw_preview_footer
    footer_y = Display.height - 14
    @sprite.line(0, footer_y, Display.width - 1, footer_y, @colors[BORDER_INDEX])
    @sprite.text(
      LABEL_X,
      footer_y + 2,
      Widget.clip(@sprite, "image: " + image_label, Display.width - LABEL_X * 2),
      @colors[TEXT_INDEX]
    )
  end
end
