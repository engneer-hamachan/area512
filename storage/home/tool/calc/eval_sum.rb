class Calc
  def parse_func
    name = ""
    while @i < @src.length
      c = cur
      c -= 32 if c >= 97 && c <= 122
      break unless c >= 65 && c <= 90
      name += c.chr
      @i += 1
    end
    raise "func" unless valid_func?(name)
    skip
    raise "func" unless cur == 40
    @i += 1
    scan_ref
    c1 = @tc
    r1 = @tr
    c2 = c1
    r2 = r1
    skip
    if cur == 58
      @i += 1
      scan_ref
      c2 = @tc
      r2 = @tr
    end
    skip
    raise "func" unless cur == 41
    @i += 1
    range_func(name, c1, r1, c2, r2)
  end

  def valid_func?(name)
    name == "SUM" || name == "AVG" || name == "AVERAGE" ||
      name == "MIN" || name == "MAX" || name == "COUNT"
  end

  def scan_ref
    skip
    c = cur
    c -= 32 if c >= 97 && c <= 122
    raise "ref" if c < 65 || c > 90
    @tc = c - 65
    @i += 1
    r = 0
    saw = false
    while @i < @src.length
      c = cur
      break unless c >= 48 && c <= 57
      saw = true
      r = r * 10 + c - 48
      @i += 1
    end
    raise "ref" unless saw
    @tr = r - 1
  end

  def range_func(name, c1, r1, c2, r2)
    minc = c1 < c2 ? c1 : c2
    maxc = c1 > c2 ? c1 : c2
    minr = r1 < r2 ? r1 : r2
    maxr = r1 > r2 ? r1 : r2
    sum = 0
    count = 0
    min = nil
    max = nil
    r = minr
    while r <= maxr
      c = minc
      while c <= maxc
        v = value_at(c, r)
        if v.nil?
          raise "func" unless name == "COUNT"
        else
          sum += v
          count += 1
          min = v if min.nil? || v < min
          max = v if max.nil? || v > max
        end
        c += 1
      end
      r += 1
    end
    return count if name == "COUNT"
    raise "func" if count == 0
    return sum if name == "SUM"
    return sum.to_f / count if name == "AVG" || name == "AVERAGE"
    return min if name == "MIN"
    max
  end

end
