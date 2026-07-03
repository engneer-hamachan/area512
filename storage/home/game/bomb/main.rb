class Bomb
  def handle(input)
    case input
    when "h"
      move_cursor(-1, 0)
    when "l"
      move_cursor(1, 0)
    when "k"
      move_cursor(0, -1)
    when "j"
      move_cursor(0, 1)
    when "y"
      move_cursor(-1, -1)
    when "u"
      move_cursor(1, -1)
    when "b"
      move_cursor(-1, 1)
    when "n"
      move_cursor(1, 1)
    when "f"
      toggle_flag
    when " ", "\r", "\n"
      open_cursor
    when "r"
      init_game
    when "q"
      @quit = true
    else
      @msg = "hjkl move, space open, f flag"
    end
  end

  def run
    init_game
    Display.fill_screen(C_BG)
    @sp = Sprite.new(Display.width, Display.height, 12)
    begin
      while !@quit
        draw
        handle(read_char)
      end
      draw_end
      read_char
    ensure
      @sp.delete if @sp
      Display.fill_screen(C_BG)
    end
  end
end

Bomb.new.run
