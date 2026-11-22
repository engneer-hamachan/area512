require 'machine'
require 'io/console'
require 'area512-sprite'
require 'area512-sdfat'
require 'area512-widget'

class Writer
  def read_key
    @list_index = nil
    if @is_keyboard_open.nil?
      @is_keyboard_open = false
      toggle_keyboard unless Touch.bluetooth_keyboard_connected?
    end
    input = nil
    while input.nil?
      push_touch_buttons(TOUCH_BUTTONS, @is_keyboard_open ? 0 : nil) if @pushed_touch_buttons != TOUCH_BUTTONS
      input = read_pending_chars(1) || read_touch_input
      sleep_ms 10 if input.nil?
    end
    STDIN.raw do
      if input == "\e"
        a = STDIN.getch
        b = STDIN.getch
        input = input + a + b if a && b
      end
    end
    @pushed_touch_buttons = nil if input == OPEN_KEY || input == SAVE_KEY
    input
  end
end
