# Solitair -- card and label rendering helpers

class Solitair
  def board_x
    (Display.width - BOARD_W) / 2
  end

  def board_y
    (Display.height - BOARD_H) / 2
  end

  def clip(s, w)
    s = "#{s}"
    return s if s.length <= w
    s[0, w]
  end

  def card_color(c)
    red_card?(c) ? C_RED : C_BLACK
  end

  def draw_label(sp, pos, x, y, text)
    col = pos == @cursor ? C_SEL : C_DIM
    sp.text(x, y, text, col)
  end

  def suit_mark(card)
    s = card_suit(card)
    return "H" if s == 1
    return "D" if s == 2
    return "C" if s == 3
    "S"
  end

  def rank_mark(card)
    r = card_rank(card)
    return "A" if r == 0
    return "#{r + 1}" if r < 9
    return "T" if r == 9
    return "J" if r == 10
    return "Q" if r == 11
    "K"
  end

  def draw_empty_slot(sp, x, y)
    sp.fill_rect(x + 1, y + 1, CARD_W - 2, CARD_H - 2, C_EMPTY)
    sp.rect(x, y, CARD_W, CARD_H, C_DIM)
  end

  def draw_card(sp, x, y, card)
    if card.is_a?(Integer)
      sp.fill_rect(x + 2, y + 2, CARD_W, CARD_H, C_SHADOW)
      if card < 0
        sp.fill_rect(x + 1, y + 1, CARD_W - 2, CARD_H - 2, C_BACK)
        sp.rect(x, y, CARD_W, CARD_H, C_TEXT)
      else
        col = card_color(card)
        sp.fill_rect(x + 1, y + 1, CARD_W - 2, CARD_H - 2, C_CARD)
        sp.rect(x, y, CARD_W, CARD_H, C_EDGE)
        sp.text(x + 3, y + 2, rank_mark(card), col)
        sp.text(x + 10, y + 11, suit_mark(card), col)
      end
    else
      draw_empty_slot(sp, x, y)
    end
  end

  def draw_scroll(sp, x, y, col, pile)
    return if pile.length <= VISIBLE_ROWS
    scroll = @scroll[col]
    scroll = 0 unless scroll.is_a?(Integer)
    max = max_scroll_for(pile)
    sx = x + CARD_W + 1
    sp.line(sx, y, sx, y + CARD_H + (VISIBLE_ROWS - 1) * STACK_STEP, C_DIM)
    sp.text(sx - 2, y - 1, "^", scroll > 0 ? C_SCROLL : C_DIM)
    sp.text(sx - 2, y + CARD_H + (VISIBLE_ROWS - 1) * STACK_STEP - 6,
            "v", scroll < max ? C_SCROLL : C_DIM)
  end
end
