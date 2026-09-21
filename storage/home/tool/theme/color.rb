class ThemeEditor
  def read_channel(color, channel_index)
    (color >> CHANNEL_SHIFTS[channel_index]) & 0xFF
  end

  def replace_channel(color, channel_index, channel_value)
    shift = CHANNEL_SHIFTS[channel_index]
    (color & ~(0xFF << shift)) | (channel_value << shift)
  end

  def format_hex_byte(byte)
    byte.to_s(16).rjust(2, "0").upcase
  end

  def format_hex_color(color)
    "0x" + format_hex_byte(read_channel(color, 0)) +
      format_hex_byte(read_channel(color, 1)) +
      format_hex_byte(read_channel(color, 2))
  end

  def parse_hex_color(text)
    upcase_text = text.upcase
    return nil unless upcase_text.length == 8
    return nil unless upcase_text.start_with?("0X")

    color = 0
    character_index = 2
    while character_index < 8
      digit = HEX_DIGITS.index(upcase_text[character_index].to_s)
      return nil unless digit

      color = color * 16 + digit
      character_index += 1
    end
    color
  end
end
