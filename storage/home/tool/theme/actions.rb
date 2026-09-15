class ThemeEditor
  def handle_key(key)
    @message = nil

    if @preview
      @preview = false if key == "\t" || key == "ESC"
      return
    end

    case key
    when "j" then move_row(1)
    when "k" then move_row(-1)
    when "h" then move_channel(-1)
    when "l" then move_channel(1)
    when "+" then adjust_channel(1)
    when "-" then adjust_channel(-1)
    when "]" then adjust_channel(16)
    when "[" then adjust_channel(-16)
    when "ENTER" then edit_row
    when "\t" then @preview = true
    when "s" then save_theme
    when "ESC" then quit
    end
  end

  def move_row(delta)
    @row_index = (@row_index + delta) % ROW_COUNT
  end

  def move_channel(delta)
    @channel_index = (@channel_index + delta) % CHANNEL_SHIFTS.length
  end

  def adjust_channel(delta)
    return if @row_index == IMAGE_ROW_INDEX

    color = @colors[@row_index]
    channel_value = read_channel(color, @channel_index) + delta
    channel_value = [[channel_value, 0].max, 255].min
    @colors[@row_index] = replace_channel(color, @channel_index, channel_value)
    @modified = true
  end

  def edit_row
    if @row_index == IMAGE_ROW_INDEX
      select_image
    else
      input_color
    end
  end

  def input_color
    text = Widget.input(@sprite, COLOR_KEYS[@row_index] + ":", format_hex_color(@colors[@row_index]))
    return unless text

    color = parse_hex_color(text)
    unless color
      @message = "Use 0xRRGGBB"
      return
    end

    @colors[@row_index] = color
    @modified = true
  end

  def select_image
    items = ["(none)"] + @image_names
    initial_index = find_item_index(items, image_label)
    selected_index = Widget.menu(@sprite, "IMAGE", items, initial_index ? initial_index : 0)
    return unless selected_index

    @background_image = selected_index == 0 ? "" : IMAGE_DIRECTORY + "/" + items[selected_index]
    @modified = true
  end

  def quit
    return if @modified && !Widget.confirm(@sprite, "Discard changes?")

    @quit = true
  end
end
