class PaintApp
  def run
    init
    Display.move_window(TOUCH_WINDOW_LEFT, Display.read_window_top_without_soft_keyboard)
    push_touch_button_column(-1)
    Display.fill_screen(BG_COLOR)
    @screen = BandedSprite.new(10)
    draw_screen
    begin
      while !@quit
        handle(read_key)
      end
      draw_goodbye
      read_key
    ensure
      @screen.delete if @screen
      Display.fill_screen(BG_COLOR)
    end
  end
end

PaintApp.new.run
