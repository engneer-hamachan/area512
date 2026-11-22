class Scheduler
  def scheduler_read_char
    buttons = @screen == :calendar ? CALENDAR_TOUCH_BUTTONS : DAY_TOUCH_BUTTONS
    push_touch_buttons(buttons) if @pushed_touch_buttons != buttons
    while true
      key = nil
      STDIN.raw { key = STDIN.read_nonblock(1) }
      return key if key
      key = read_touch_key
      return key if key
      sleep_ms 10
    end
  end

  def read_typed_char
    value = nil
    STDIN.raw { value = STDIN.getch }
    value.is_a?(String) ? value : ""
  end

  def read_entry(label, initial = "")
    is_keyboard_opened = !Touch.bluetooth_keyboard_connected?
    Touch.open_keyboard if is_keyboard_opened
    push_touch_button_column(-1) if is_keyboard_opened
    text = read_entry_text(label, initial)
    Touch.close_keyboard if is_keyboard_opened
    push_touch_button_column(-1) if is_keyboard_opened
    discard_touch_events
    text
  end

  def read_entry_text(label, initial)
    text = initial
    loop do
      draw_entry(label, text)
      input = read_typed_char
      code = input.length > 0 ? input.ord : 0
      return text if code == 10 || code == 13
      return initial if code == 27
      if code == 0
        # Ctrl+Space IME switching is handled by the keyboard layer.
        next
      elsif code == 8 || code == 127
        text = drop_last_char(text)
      elsif (code >= 32 || input.length > 1) && char_count(text) < 28
        text += input
      end
    end
  end

  def utf8_step(text, pos)
    byte = text.getbyte(pos)
    return 1 unless byte.is_a?(Integer)
    return 1 if byte < 0x80
    return 2 if byte < 0xe0
    return 3 if byte < 0xf0
    4
  end

  def char_count(text)
    pos = 0
    count = 0
    while pos < text.length
      pos += utf8_step(text, pos)
      count += 1
    end
    count
  end

  def drop_last_char(text)
    return "" if text.length == 0
    pos = 0
    previous = 0
    while pos < text.length
      previous = pos
      pos += utf8_step(text, pos)
    end
    part = text[0, previous]
    part.is_a?(String) ? part : ""
  end

  def clip_text(text, max_width)
    pos = 0
    width = 0
    while pos < text.length
      byte = text.getbyte(pos)
      char_width = 6
      char_width = 12 if byte.is_a?(Integer) && byte >= 0x80
      break if width + char_width > max_width
      width += char_width
      pos += utf8_step(text, pos)
    end
    part = text[0, pos]
    part.is_a?(String) ? part : ""
  end
end
