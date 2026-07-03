class PaintApp
  def run
    init
    Display.fill_screen(BG_COLOR)
    @screen = Sprite.new(APP_W, APP_H)
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
