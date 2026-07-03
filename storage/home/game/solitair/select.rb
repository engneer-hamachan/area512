# Solitair -- stock and selection actions

class Solitair
  def draw_stock
    clear_sel
    if @stock.empty?
      while !@waste.empty?
        @stock.push(@waste.pop)
      end
      @msg = "Stock reset."
    else
      n = 0
      while n < 3 && !@stock.empty?
        @waste.push(@stock.pop)
        n += 1
      end
      @msg = n == 1 ? "Drew 1 card." : "Drew #{n} cards."
    end
  end

  def flip_top(col)
    pile = @tab[col]
    return false if pile.empty?
    top = pile[pile.length - 1]
    return false if face_up?(top)
    pile[pile.length - 1] = card_id(top)
    ensure_card_visible(col, pile.length - 1)
    @moves += 1
    @msg = "Flipped #{card_name(pile[pile.length - 1])}."
    true
  end

  def select_waste
    if @waste.empty?
      @msg = "Waste is empty."
    else
      @sel = 1
      @sel_idx = @waste.length - 1
      @msg = "Selected #{card_name(@waste[@sel_idx])}."
    end
  end

  def select_tableau
    col = cursor_col
    pile = @tab[col]
    if pile.empty?
      @msg = "Column is empty."
    elsif !face_up?(pile[pile.length - 1])
      flip_top(col)
    else
      i = first_face_up_index(pile)
      @sel = @cursor
      @sel_idx = i
      ensure_card_visible(col, @sel_idx)
      @msg = "Selected #{pile.length - @sel_idx} cards."
    end
  end

  def select_foundation
    suit = @cursor - 2
    count = @found[suit]
    count = 0 unless count.is_a?(Integer)
    if count <= 0
      @msg = "Foundation is empty."
    else
      @sel = @cursor
      @sel_idx = count - 1
      @msg = "Selected #{card_name(suit * 13 + count - 1)}."
    end
  end

  def select_here
    if @cursor == 0
      draw_stock
    elsif @cursor == 1
      select_waste
    elsif @cursor >= 2 && @cursor <= 5
      select_foundation
    elsif cursor_tableau?
      select_tableau
    else
      @msg = "No selectable card."
    end
  end

  def adjust_selection(delta)
    return false unless @sel >= 6
    pile = @tab[@sel - 6]
    ni = @sel_idx + delta
    return false if ni < 0 || ni >= pile.length
    return false unless can_stack_from?(pile, ni)
    @sel_idx = ni
    ensure_card_visible(@sel - 6, @sel_idx)
    @msg = "Selected #{pile.length - @sel_idx} cards."
    true
  end
end
