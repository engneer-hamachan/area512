class Breadboard
  def initialize
    @parts = []
    @cursor_row = 2
    @cursor_column = 0
    @placing_entry = nil
    @placing_first_hole_index = nil
    @menu_category_index = 0
    @menu_entry_index = 0
    @message = ""
    @help_visible = false
    @quit = false
    @needs_redraw = true
    @frame_count = 0
    update_circuit_structure
  end

  def run
    Display.fill_screen(Widget.theme_background)
    @sprite = BandedSprite.new(12)
    begin
      while !@quit
        key = read_pending_key
        handle_key(key) if key
        step_simulation
        draw_screen if @needs_redraw || animation_running?
        @needs_redraw = false
        @frame_count += 1
        sleep_ms(50)
      end
    ensure
      @sprite.delete if @sprite
      Display.fill_screen(Widget.theme_background)
    end
  end
end

Breadboard.new.run
