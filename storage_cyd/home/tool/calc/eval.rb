class Calc
  def eval_formula(raw)
    old_src = @src
    old_i = @i
    @src = raw
    @i = 1
    v = parse_expr
    skip
    raise "extra" if @i < @src.length
    @src = old_src
    @i = old_i
    v
  rescue
    @src = old_src
    @i = old_i
    nil
  end

  def cell_ref?(s)
    return false if s.length < 2
    c = s.getbyte(0)
    c -= 32 if c >= 97 && c <= 122
    return false if c < 65 || c > 90
    i = 1
    while i < s.length
      c = s.getbyte(i)
      return false unless c >= 48 && c <= 57
      i += 1
    end
    s[1, s.length - 1].to_i > 0
  end

  def ref_pos(s)
    c = s.getbyte(0)
    c -= 32 if c >= 97 && c <= 122
    [c - 65, s[1, s.length - 1].to_i - 1]
  end

  def value_at(c, r)
    return nil if r < 0 || r >= ROWS || c < 0 || c >= COLS
    @values[cell_key(c, r)]
  end
end
