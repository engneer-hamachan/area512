class Scheduler
  def handle(input)
    if input == "s"
      save_data
    elsif input == "q"
      @quit = true
    elsif @screen == :calendar
      handle_calendar(input)
    else
      handle_day(input)
    end
  end

  def handle_calendar(input)
    case input
    when "h" then move_day(-1)
    when "l" then move_day(1)
    when "k" then move_day(-7)
    when "j" then move_day(7)
    when "[" then move_month(-1)
    when "]" then move_month(1)
    when "\r", "\n", " "
      @screen = :day
      reset_event_cursor
      @msg = "a add  d delete  c calendar"
    else
      @msg = "hjkl move [] month ENTER day s save"
    end
  end

  def handle_day(input)
    case input
    when "h" then move_day(-1)
    when "l" then move_day(1)
    when "j" then @event_cursor += 1
    when "k" then @event_cursor -= 1
    when "a" then add_event
    when "d" then delete_event
    when "c", "\e"
      @screen = :calendar
      @msg = "ENTER day  s save"
    else
      @msg = "h/l date j/k select a add d del c back"
    end
    clamp_event_cursor
  end

  def add_event
    time = read_entry("Time (HH:MM)", "09:00")
    return unless valid_time?(time)
    text = read_entry("Schedule", "")
    return if text.length == 0
    day_events.push([time, text])
    day_events.sort! { |a, b| a[0] <=> b[0] }
    @event_cursor = day_events.length - 1
    @msg = "Added " + time
  end

  def valid_time?(text)
    parts = text.split(":")
    ok = parts.length == 2 && parts[0].length == 2 && parts[1].length == 2
    ok = false if ok && (parts[0].to_i > 23 || parts[1].to_i > 59)
    @msg = "Use HH:MM" unless ok
    ok
  end

  def delete_event
    return if day_events.empty?
    day_events.delete_at(@event_cursor)
    @msg = "Deleted"
  end

  def run
    Display.fill_screen(C_BG)
    @sp = Sprite.new(W, H, 12)
    begin
      SD.mount
      SD.mkdir(DATA_DIR)
      load_data
      until @quit
        draw
        handle(scheduler_read_char)
      end
    ensure
      SD.unmount
      @sp.delete if @sp
      Display.fill_screen(C_BG)
    end
  end
end

Scheduler.new.run
