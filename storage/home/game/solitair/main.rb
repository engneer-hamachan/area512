class Solitair
  def handle_number(input)
    if input == "0"
      @cursor = 1
      select_waste if @sel < 0
      return true
    end
    col = input.to_i - 1
    return false if col < 0 || col > 6
    @cursor = col + 6
    ensure_card_visible(col, @tab[col].length - 1)
    if @sel >= 0
      reason = explain_tableau_move(col)
      return true if move_to_tableau(col)
      @msg = reason
      clear_sel
    else
      select_tableau
    end
    true
  end

  def move_selected_to_foundation
    if @sel >= 2 && @sel <= 5
      @msg = "Already home."
      clear_sel
      return true
    end
    selected = selected_card
    if selected.is_a?(Integer) && selected >= 0
      suit = card_suit(selected)
      return true if move_to_foundation(suit)
      @msg = "Cannot move #{card_name(selected)} home."
      clear_sel
      return true
    end
    if @cursor >= 2 && @cursor <= 5
      @cursor += 1
      @cursor = 2 if @cursor > 5
    else
      @cursor = 2
    end
    @msg = "Foundation #{@cursor - 1}."
    true
  end

  def handle(input)
    return if handle_number(input)
    case input
    when "h"
      move_cursor(-1)
    when "l"
      move_cursor(1)
    when "j"
      if @sel >= 0
        @msg = "Cannot narrow." unless adjust_selection(1)
      else
        scroll_col(1)
      end
    when "k"
      if @sel >= 0
        @msg = "Cannot widen." unless adjust_selection(-1)
      else
        scroll_col(-1)
      end
    when "d"
      draw_stock
    when " ", "\r", "\n"
      try_move
    when "a"
      auto_foundation
    when "f"
      move_selected_to_foundation
    when "n"
      init_game
    when "q"
      @quit = true
    else
      @msg = "h/l cursor, d draw, space move, k/j stack"
    end
  end

  def run
    Display.fill_screen(C_BG)
    @sp = Sprite.new(Display.width, Display.height, 12)
    begin
      SD.mount
      load_high_score
      init_game
      while !@quit
        draw
        handle(read_char)
      end
      draw_end
      read_char
    ensure
      SD.unmount
      @sp.delete if @sp
      Display.fill_screen(C_BG)
    end
  end
end

Solitair.new.run

puts "Bye."
