require 'area512-sprite'
require 'area512-widget'
require 'area512-sdfat'
require 'io/console'

class ThemeEditor
  def read_key
    key = ""
    STDIN.raw do
      key = Widget.read_key
    end
    key
  end

  def run
    @row_index = 0
    @channel_index = 0
    @preview = false
    @modified = false
    @message = nil
    @quit = false

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
