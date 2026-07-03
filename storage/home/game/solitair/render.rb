# Solitair -- board rendering

class Solitair
  def draw_top
    sp = @sp
    ox = board_x
    oy = board_y
    y = oy + TOP_Y
    draw_label(sp, 0, ox + 5, oy, "ST")
    if @stock.empty?
      draw_empty_slot(sp, ox, y)
    else
      draw_card(sp, ox, y, -1)
    end
    draw_label(sp, 1, ox + COL_W + 5, oy, "WA")
    draw_waste(sp, ox, y)
    s = 0
    while s < 4
      x = ox + (s + 3) * COL_W
      draw_label(sp, s + 2, x + 5, oy, "F#{s + 1}")
      if @found[s] == 0
        draw_empty_slot(sp, x, y)
      else
        draw_card(sp, x, y, s * 13 + @found[s] - 1)
      end
      if @sel == s + 2 && @found[s] > 0
        sp.rect(x - 1, y - 1, CARD_W + 2, CARD_H + 2, C_SEL)
      end
      s += 1
    end
  end

  def draw_tableau
    sp = @sp
    ox = board_x
    oy = board_y
    c = 0
    while c < 7
      x = ox + c * COL_W
      y = oy + TABLEAU_Y
      pile = @tab[c]
      label = column_label(c, pile)
      draw_label(sp, c + 6, centered_col_label_x(x, label), y - CH - 4, label)
      r = 0
      clamp_scroll(c)
      scroll = @scroll[c]
      scroll = 0 unless scroll.is_a?(Integer)
      while r < VISIBLE_ROWS
        if pile.length <= VISIBLE_ROWS
          i = r
        elsif r == 0
          i = 0
        elsif r == VISIBLE_ROWS - 1
          i = pile.length - 1
        else
          i = scroll + r
        end
        break if i >= pile.length
        cy = y + r * STACK_STEP
        draw_card(sp, x, cy, pile[i])
        if @sel == c + 6 && i >= @sel_idx
          sp.rect(x - 1, cy - 1, CARD_W + 2, CARD_H + 2, C_SEL)
        end
        r += 1
      end
      draw_empty_slot(sp, x, y) if pile.empty?
      draw_scroll(sp, x, y, c, pile)
      c += 1
    end
    if @sel == 1 && !@waste.empty?
      x = waste_card_x(ox, @waste.length - 1)
      sp.rect(x - 1, oy + TOP_Y - 1, CARD_W + 2, CARD_H + 2, C_SEL)
    end
  end

  def draw
    sp = @sp
    ox = board_x
    oy = board_y
    sp.fill(C_BG)
    sp.fill_rect(ox - CW, oy, BOARD_W + CW, TABLEAU_Y - 8, C_PANEL)
    draw_top
    draw_tableau
    sp.push(0, 0)
  end

  # Center a vertical list of [text, color] lines on the screen.
  # ASCII glyphs are 6px wide in efontJA_12; "" rows act as spacers.
  def draw_centered(sp, lines, lh)
    y = (Display.height - lines.length * lh) / 2
    lines.each do |line|
      s = "#{line[0]}"
      sp.text((Display.width - s.length * 6) / 2, y, s, line[1])
      y += lh
    end
  end

  def draw_end
    sp = @sp
    sp.fill(C_BG)
    draw_centered(sp, [
      ["Solitair", C_SEL],
      ["", C_TEXT],
      [@won ? "Complete." : "Bye.", C_TEXT],
      ["Moves #{@moves}", C_DIM],
      ["Time #{format_time(@elapsed)}", C_DIM],
      ["Score #{@score}", C_DIM],
      ["Best #{@high_score}", C_DIM]
    ], CH + 4)
    sp.push(0, 0)
  end
end
