require 'area512-sprite'
require 'area512-widget'
require 'io/console'

class WidgetGallery
  def read_key
    key = ""

    STDIN.raw do
      key = Widget.read_key
    end

    key
  end

  def run
    Display.fill_screen(Widget.bg)
    @sp = Sprite.new(Display.width, Display.height, 12)

    begin
      until @quit
        draw
        handle(read_key)
      end
    ensure
      @sp.delete if @sp
      Display.fill_screen(Widget.bg)
    end
  end
end

WidgetGallery.new.run
