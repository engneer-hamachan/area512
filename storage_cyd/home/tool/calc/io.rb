require 'machine'
require 'io/console'
require 'area512-touch'

class Calc
  def read_char
    draw_touch_buttons
    while true
      character = read_pending_character
      if character
        close_keyboard if @editing && @keyboard_open && (character == "\r" || character == "\n" || character == "\e")
        return character
      end

      event = read_touch_event
      if event
        key = apply_touch_event(event)
        return key if key
      else
        sleep_ms 10
      end
    end
  end

  def read_pending_character
    character = nil
    STDIN.raw do
      character = STDIN.read_nonblock(1)
    end
    character
  end

  def read_typed_character
    character = nil
    STDIN.raw do
      character = STDIN.getch
    end
    character ? character : ""
  end

  def read_entry(prompt, initial)
    keyboard_opened_for_entry = !@keyboard_open && !Touch.bluetooth_keyboard_connected?
    if keyboard_opened_for_entry
      open_keyboard
      draw
      draw_touch_buttons
    end
    entry = read_typed_entry(prompt, initial)
    close_keyboard if keyboard_opened_for_entry
    discard_touch_events
    entry
  end

  def read_typed_entry(prompt, initial)
    print prompt
    print initial
    buf = initial
    loop do
      ch = read_typed_character
      next if ch.length == 0
      code = ch.ord
      if code == 13 || code == 10
        puts
        return buf
      elsif code == 27
        puts
        return initial
      elsif code == 8 || code == 127
        if buf.length > 0
          cut = buf[0, buf.length - 1]
          buf = cut if cut.is_a?(String)
          print "\b \b"
        end
      elsif code >= 32 && code < 127
        buf = buf + ch
        print ch
      end
    end
  end
end
