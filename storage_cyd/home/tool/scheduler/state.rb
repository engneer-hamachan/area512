class Scheduler
  def initialize
    @year = 2026
    @month = 6
    @day = 21
    @screen = :calendar
    @events = {}
    @event_cursor = 0
    @event_top = 0
    @msg = "ENTER day  s save"
    @quit = false
  end

  def day_events
    list = @events[date_key]
    unless list.is_a?(Array)
      list = []
      @events[date_key] = list
    end
    list
  end

  def reset_event_cursor
    @event_cursor = 0
    @event_top = 0
  end

  def clamp_event_cursor
    list = day_events
    @event_cursor = 0 if @event_cursor < 0
    @event_cursor = list.length - 1 if @event_cursor >= list.length && !list.empty?
    @event_cursor = 0 if list.empty?
    @event_top = @event_cursor if @event_cursor < @event_top
    @event_top = @event_cursor - 5 if @event_cursor >= @event_top + 6
  end
end
