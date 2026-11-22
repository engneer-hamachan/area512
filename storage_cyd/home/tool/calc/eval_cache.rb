class Calc
  def init_values
    @values = {}
    @dirty = true
  end

  def raw_num(s)
    return 0 if s.length == 0
    neg = false
    i = 0
    if s.getbyte(0) == 45
      neg = true
      i = 1
    end
    v = 0
    div = 0
    while i < s.length
      c = s.getbyte(i)
      if c >= 48 && c <= 57
        if div == 0
          v = v * 10 + c - 48
        else
          v += (c - 48).to_f / div
          div *= 10
        end
      elsif c == 46 && div == 0
        div = 10
      else
        break
      end
      i += 1
    end
    neg ? -v : v
  end

  def recalc_sheet
    @values = {}
    r = 0
    while r < ROWS
      c = 0
      while c < COLS
        raw = cell_at(c, r)
        if raw.length > 0 && !raw.start_with?("=")
          @values[cell_key(c, r)] = raw_num(raw)
        end
        c += 1
      end
      r += 1
    end
    pass = 0
    while pass < 6
      recalc_formulas
      pass += 1
    end
    @dirty = false
  end

  def recalc_formulas
    r = 0
    while r < ROWS
      c = 0
      while c < COLS
        raw = cell_at(c, r)
        if raw.start_with?("=")
          @values[cell_key(c, r)] = eval_formula(raw)
        end
        c += 1
      end
      r += 1
    end
  end
end
