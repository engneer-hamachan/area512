class Calc
  def cur
    return 0 if @i >= @src.length
    @src.getbyte(@i)
  end

  def skip
    while @i < @src.length
      c = cur
      break unless c == 32 || c == 9
      @i += 1
    end
  end

  def parse_expr
    v = parse_term
    skip
    while cur == 43 || cur == 45
      op = cur
      @i += 1
      n = parse_term
      v = op == 43 ? v + n : v - n
      skip
    end
    v
  end

  def parse_term
    v = parse_factor
    skip
    while cur == 42 || cur == 47
      op = cur
      @i += 1
      n = parse_factor
      v = op == 42 ? v * n : (n == 0 ? 0 : v / n)
      skip
    end
    v
  end

  def parse_factor
    skip
    sign = 1
    while cur == 43 || cur == 45
      sign = -sign if cur == 45
      @i += 1
      skip
    end
    c = cur
    if c == 40
      @i += 1
      v = parse_expr
      skip
      raise "paren" unless cur == 41
      @i += 1
    else
      c -= 32 if c >= 97 && c <= 122
      if c >= 65 && c <= 90
        n = 0
        n = @src.getbyte(@i + 1) if @i + 1 < @src.length
        if n >= 48 && n <= 57
          v = parse_ref
        else
          v = parse_func
        end
      else
        v = parse_num
      end
    end
    sign * v
  end

  def parse_num
    v = 0
    div = 0
    saw = false
    while @i < @src.length
      c = cur
      if c >= 48 && c <= 57
        saw = true
        v = div == 0 ? v * 10 + c - 48 : v + (c - 48).to_f / div
        div *= 10 if div > 0
        @i += 1
      elsif c == 46 && div == 0
        div = 10
        @i += 1
      else
        break
      end
    end
    raise "num" unless saw
    v
  end

  def parse_ref
    c = cur
    c -= 32 if c >= 97 && c <= 122
    @i += 1
    r = 0
    saw = false
    while @i < @src.length
      d = cur
      break unless d >= 48 && d <= 57
      saw = true
      r = r * 10 + d - 48
      @i += 1
    end
    raise "ref" unless saw
    v = value_at(c - 65, r - 1)
    raise "ref" if v.nil?
    v
  end
end
