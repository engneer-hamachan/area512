require 'area512-sprite'
require 'area512-widget'
require 'area512-sdfat'
require 'machine'
require 'io/console'

class ThemeEditor
  def read_key
    draw_touch_buttons
    while true
      character = read_pending_character
      return read_widget_key(character) if character

      event = read_touch_event
      if event
        key = apply_touch_event(event)
        return key if key
      else
        sleep_ms 10
      end
    end
  end

  def run
    @row_index = 0
    @top_row_index = 0
    @channel_index = 0
    @preview = false
    @modified = false
    @message = nil
    @quit = false

    Display.move_window(TOUCH_WINDOW_LEFT, Display.read_window_top_without_soft_keyboard)

    push_touch_button_column(-1)
    Display.fill_screen(Widget.theme_background)
    @sprite = BandedSprite.new(12)

    begin
      SD.mount
      load_theme
      load_image_names
      until @quit
        draw
        handle_key(read_key)
      end
    ensure
      SD.unmount
      @sprite.delete if @sprite
      Display.fill_screen(Widget.theme_background)
    end
  end
end

ThemeEditor.new.run
