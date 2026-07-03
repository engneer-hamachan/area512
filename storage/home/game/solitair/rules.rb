# Solitair -- Klondike shared rules

class Solitair
  def foundation_accept?(card, suit)
    return false unless card.is_a?(Integer)
    card_suit(card) == suit && card_rank(card) == @found[suit]
  end

  def tableau_accept?(card, col)
    if card.is_a?(Integer)
      pile = @tab[col]
      return card_rank(card) == 12 if pile.empty?
      top = pile[pile.length - 1]
      if top.is_a?(Integer)
        return false unless face_up?(top)
        return red_card?(card) != red_card?(top) && card_rank(card) + 1 == card_rank(top)
      end
    end
    false
  end

  def can_stack_from?(pile, idx)
    return false if idx < 0 || idx >= pile.length
    first = pile[idx]
    return false unless first.is_a?(Integer)
    return false unless face_up?(first)
    i = idx
    while i < pile.length - 1
      a = pile[i]
      b = pile[i + 1]
      if a.is_a?(Integer) && b.is_a?(Integer)
        return false if red_card?(a) == red_card?(b)
        return false if card_rank(a) != card_rank(b) + 1
      else
        return false
      end
      i += 1
    end
    true
  end
end
