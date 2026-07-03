# Solitair -- moving selected cards

class Solitair
  def selected_card
    if @sel == 1 && !@waste.empty?
      card = @waste[@waste.length - 1]
      return card if card.is_a?(Integer)
    end
    if @sel >= 2 && @sel <= 5
      suit = @sel - 2
      count = @found[suit]
      count = 0 unless count.is_a?(Integer)
      return suit * 13 + count - 1 if count > 0
    end
    if @sel >= 6
      pile = @tab[@sel - 6]
      if @sel_idx >= 0 && @sel_idx < pile.length
        card = pile[@sel_idx]
        return card if card.is_a?(Integer)
      end
    end
    -1
  end

  def reverse_moving
    out = []
    while !@moving.empty?
      out.push(@moving.pop)
    end
    @moving = out
  end

  def remove_selected
    if @sel == 1
      card = @waste.pop
      @moving.push(card) if card.is_a?(Integer)
    elsif @sel >= 2 && @sel <= 5
      suit = @sel - 2
      count = @found[suit]
      count = 0 unless count.is_a?(Integer)
      if count > 0
        @moving.push(suit * 13 + count - 1)
        @found[suit] = count - 1
      end
    elsif @sel >= 6
      pile = @tab[@sel - 6]
      while pile.length > @sel_idx
        @moving.push(pile.pop)
      end
      reverse_moving
    end
  end

  def move_to_foundation(suit)
    return false if @sel >= 2 && @sel <= 5
    return false if @sel >= 6 && @tab[@sel - 6].length - @sel_idx > 1
    selected = selected_card
    if selected.is_a?(Integer)
      card = selected
      if card >= 0 && foundation_accept?(card, suit)
        @moving = []
        remove_selected
        count = @found[suit]
        count = 0 unless count.is_a?(Integer)
        @found[suit] = count + 1
        @moves += 1
        @msg = "Moved #{card_name(card)} home."
        clear_sel
        check_win
        return true
      end
    end
    false
  end

  def move_to_tableau(col)
    selected = selected_card
    if selected.is_a?(Integer)
      card = selected
      return false if card < 0
      return false unless tableau_accept?(card, col)
      if @sel >= 6
        return false unless can_stack_from?(@tab[@sel - 6], @sel_idx)
      end
      from_col = @sel >= 6 ? @sel - 6 : -1
      @moving = []
      remove_selected
      if from_col >= 0
        from_pile = @tab[from_col]
        ensure_card_visible(from_col, first_face_up_index(from_pile))
      end
      pile = @tab[col]
      i = 0
      while i < @moving.length
        pile.push(@moving[i])
        i += 1
      end
      ensure_card_visible(col, first_face_up_index(pile))
      @moves += 1
      @msg = "Moved #{card_name(card)}."
      clear_sel
      return true
    end
    false
  end

  def explain_tableau_move(col)
    selected = selected_card
    if selected.is_a?(Integer)
      return "Select a face-up card first." if selected < 0
      pile = @tab[col]
      if pile.empty?
        return card_rank(selected) == 12 ? "Move to empty column." : "Only K starts empty column."
      end
      top = pile[pile.length - 1]
      return "Target card is face down." unless top.is_a?(Integer) && face_up?(top)
      return "Need opposite color." if red_card?(selected) == red_card?(top)
      return "Need one rank lower." unless card_rank(selected) + 1 == card_rank(top)
      return "Move to column #{col + 1}."
    end
    "Select a face-up card first."
  end

  def try_move
    if @sel < 0
      select_here
      return
    end
    if @cursor >= 2 && @cursor <= 5
      return if move_to_foundation(@cursor - 2)
    elsif cursor_tableau?
      reason = explain_tableau_move(cursor_col)
      return if move_to_tableau(cursor_col)
      @msg = reason
      clear_sel
      return
    end
    @msg = "That move is not legal."
    clear_sel
  end
end
