class ThemeEditor
  def load_theme
    @colors = DEFAULT_COLORS.dup
    @background_image = ""
    return unless SD.exist?(THEME_PATH)

    SD.read(THEME_PATH).split("\n").each do |line|
      assign_theme_line(line.strip)
    end
  rescue
    @message = "Load failed: " + THEME_PATH
  end

  def assign_theme_line(line)
    return unless line.index("=")

    line_parts = line.split("=", 2)
    key = line_parts[0].to_s
    value = line_parts.length == 2 ? line_parts[1].to_s : ""

    if key == "background_image"
      @background_image = value
      return
    end

    color_index = find_item_index(COLOR_KEYS, key)
    color = parse_hex_color(value)
    @colors[color_index] = color if color_index && color
  end

  def find_item_index(items, item)
    item_index = 0
    while item_index < items.length
      return item_index if items[item_index] == item

      item_index += 1
    end
    nil
  end

  def load_image_names
    @image_names = []
    directory = Dir.open(IMAGE_DIRECTORY)
    begin
      while entry_name = directory.read
        @image_names.push(entry_name) if entry_name.end_with?(IMAGE_EXTENSION)
      end
    ensure
      directory.close
    end
    @image_names.sort!
  rescue
    @image_names = []
  end

  def save_theme
    content = COLOR_KEYS[BACKGROUND_INDEX] + "=" + format_hex_color(@colors[BACKGROUND_INDEX]) + "\n"
    content += "background_image=" + @background_image + "\n"
    color_index = TEXT_INDEX
    while color_index < COLOR_KEYS.length
      content += COLOR_KEYS[color_index] + "=" + format_hex_color(@colors[color_index]) + "\n"
      color_index += 1
    end

    SD.write(THEME_PATH, content)
    @modified = false
    @message = "Saved. Reboot to apply"
  rescue
    @message = "Save failed"
  end
end
