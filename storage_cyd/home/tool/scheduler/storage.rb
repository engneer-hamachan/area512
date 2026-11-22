class Scheduler
  def save_data
    data = "CURSOR\t" + date_key + "\n"
    @events.keys.sort.each do |key|
      list = @events[key]
      list.each do |event|
        data += key + "\t" + event[0] + "\t" + event[1] + "\n"
      end
    end
    SD.write(SAVE_FILE, data)
    @msg = "Saved " + date_key
  rescue
    @msg = "Save failed"
  end

  def load_data
    return unless SD.exist?(SAVE_FILE)
    SD.read(SAVE_FILE).split("\n").each { |line| load_line(line.chomp) }
    sort_events
    @msg = "Opened at " + date_key
  rescue
    @msg = "Open failed"
  end

  def load_line(line)
    parts = line.split("\t", 3)
    if parts[0] == "CURSOR" && parts.length >= 2
      set_date(parts[1])
    elsif parts.length == 3
      list = @events[parts[0]]
      list = [] unless list.is_a?(Array)
      list.push([parts[1], parts[2]])
      @events[parts[0]] = list
    end
  end

  def set_date(text)
    parts = text.split("-")
    return unless parts.length == 3
    y = parts[0].to_i
    m = parts[1].to_i
    d = parts[2].to_i
    return if y < 1 || m < 1 || m > 12
    return if d < 1 || d > days_in_month(y, m)
    @year, @month, @day = y, m, d
  end

  def sort_events
    @events.each do |_key, list|
      list.sort! { |a, b| a[0] <=> b[0] }
    end
  end
end
