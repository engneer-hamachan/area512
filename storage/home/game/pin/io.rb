require 'io/console'

class Breadboard
  def read_pending_key
    key_bytes = STDIN.read_nonblock(8)
    return nil if key_bytes.nil?

    if key_bytes == 27.chr
      sleep_ms(20)
      following_bytes = STDIN.read_nonblock(8)
      key_bytes = key_bytes + following_bytes if following_bytes
    end

    decode_key(key_bytes)
  end

  def decode_key(key_bytes)
    first_character = key_bytes[0]
    return decode_arrow_key(key_bytes[2]) if first_character == 27.chr && key_bytes[1] == "["
    return "ESC" if first_character == 27.chr

    case first_character
    when "k", ";"
      "UP"
    when "j", "."
      "DOWN"
    when "h", ","
      "LEFT"
    when "l", "/"
      "RIGHT"
    when "\r", "\n"
      "ENTER"
    when "\b", 127.chr
      "BS"
    else
      first_character
    end
  end

  def decode_arrow_key(final_character)
    case final_character
    when "A"
      "UP"
    when "B"
      "DOWN"
    when "C"
      "RIGHT"
    when "D"
      "LEFT"
    else
      ""
    end
  end
end
