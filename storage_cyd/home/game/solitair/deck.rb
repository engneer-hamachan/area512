# Solitair -- deck setup and cursor helpers

class Solitair
  def new_deck
    d = []
    i = 0
    while i < 52
      d.push(i)
      i += 1
    end
    i = d.length - 1
    while i > 0
      j = rnd(i + 1)
      t = d[i]
      d[i] = d[j]
      d[j] = t
      i -= 1
    end
    d
  end

  def init_game
    d = new_deck
    @tab = []
    c = 0
    while c < 7
      pile = []
      n = 0
      while n <= c
        card = d.pop
        card = 0 unless card.is_a?(Integer)
        card = -card - 1 if n < c
        pile.push(card)
        n += 1
      end
      @tab.push(pile)
      c += 1
    end
    @stock = d
    @waste = []
    @found = [0, 0, 0, 0]
    @scroll = [0, 0, 0, 0, 0, 0, 0]
    c = 0
    while c < 7
      ensure_card_visible(c, @tab[c].length - 1)
      c += 1
    end
    @cursor = 0
    @sel = -1
    @sel_idx = -1
    @moves = 0
    @score = 0
    @elapsed = 0
    @start_us = Machine.uptime_us
    @won_saved = false
    @quit = false
    @won = false
    @msg = "d draw, space move, k/j stack, a auto, n new, q quit"
  end

  def cursor_tableau?
    @cursor >= 6
  end

  def cursor_col
    @cursor - 6
  end

  def move_cursor(dx)
    @cursor += dx
    @cursor = 12 if @cursor < 0
    @cursor = 0 if @cursor > 12
    if cursor_tableau?
      col = cursor_col
      pile = @tab[col]
      ensure_card_visible(col, pile.length - 1)
    end
  end

  def clear_sel
    @sel = -1
    @sel_idx = -1
  end

  def clamp_scroll(col)
    pile = @tab[col]
    max = max_scroll_for(pile)
    scroll = @scroll[col]
    scroll = 0 unless scroll.is_a?(Integer)
    scroll = 0 if scroll < 0
    scroll = max if scroll > max
    @scroll[col] = scroll
  end

  def max_scroll_for(pile)
    return 0 if pile.length <= VISIBLE_ROWS
    max = pile.length - (VISIBLE_ROWS - 2) - 2
    max = 0 if max < 0
    max
  end

  def first_face_up_index(pile)
    i = 0
    while i < pile.length && !face_up?(pile[i])
      i += 1
    end
    i
  end

  def ensure_card_visible(col, idx)
    return if idx < 0
    clamp_scroll(col)
    pile = @tab[col]
    return if pile.length <= VISIBLE_ROWS
    return if idx == 0 || idx == pile.length - 1
    scroll = @scroll[col]
    scroll = 0 unless scroll.is_a?(Integer)
    middle_slots = VISIBLE_ROWS - 2
    middle_start = scroll + 1
    middle_bottom = middle_start + middle_slots - 1
    scroll = idx - 1 if idx < middle_start
    scroll = idx - middle_slots if idx > middle_bottom
    @scroll[col] = scroll
    clamp_scroll(col)
  end

  def scroll_col(delta)
    return false unless cursor_tableau?
    col = cursor_col
    before = @scroll[col]
    before = 0 unless before.is_a?(Integer)
    @scroll[col] = before + delta
    clamp_scroll(col)
    if @scroll[col] == before
      @msg = delta < 0 ? "Top of column." : "Bottom of column."
      return false
    end
    @msg = "Scrolled column #{col + 1}."
    true
  end
end
