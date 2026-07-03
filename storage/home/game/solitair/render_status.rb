# Solitair -- rendering state helpers

class Solitair
  def waste_start
    n = @waste.length - 3
    n < 0 ? 0 : n
  end

  def waste_card_x(ox, i)
    ox + COL_W + (i - waste_start) * 8
  end

  def draw_waste(sp, ox, y)
    if @waste.empty?
      draw_empty_slot(sp, ox + COL_W, y)
    else
      i = waste_start
      while i < @waste.length
        draw_card(sp, waste_card_x(ox, i), y, @waste[i])
        i += 1
      end
    end
  end

  def action_hint
    if @sel >= 0
      card = selected_card
      name = "card"
      if card.is_a?(Integer)
        name = card_name(card) if card >= 0
      end
      return "#{name}: 1-7 move f home j/k size"
    end
    if @cursor == 0
      return "ST: d draw 0 waste f foundation"
    end
    if @cursor == 1
      return "WA: 0 select top 1-7 move"
    end
    if @cursor >= 2 && @cursor <= 5
      return "F#{@cursor - 1}: sp select top f next"
    end
    "Col #{cursor_col + 1}: 1-7 select/move f foundation"
  end

  def status_line
    hi = @high_score
    hi = 0 unless hi.is_a?(Integer)
    "#{action_hint}  T#{format_time(elapsed_seconds)} Hi#{hi}  #{@msg}"
  end

  def face_down_count(pile)
    n = 0
    i = 0
    while i < pile.length
      card = pile[i]
      n += 1 if card.is_a?(Integer) && card < 0
      i += 1
    end
    n
  end

  def column_label(col, pile)
    down = face_down_count(pile)
    label = "#{col + 1}"
    label = "#{label}(#{down})" if down > 0
    label
  end

  def centered_col_label_x(x, label)
    x + (CARD_W - label.length * CW) / 2
  end
end
