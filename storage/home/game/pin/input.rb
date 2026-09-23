class Breadboard
  def handle_key(key)
    @needs_redraw = true
    @message = ""
    if @help_visible
      @help_visible = false
      return
    end
    return if move_cursor_by_key(key)
    if @placing_entry
      handle_placement_key(key)
    else
      handle_edit_key(key)
    end
  end

  def move_cursor_by_key(key)
    case key
    when "UP"
      move_cursor(0, -1)
    when "DOWN"
      move_cursor(0, 1)
    when "LEFT"
      move_cursor(-1, 0)
    when "RIGHT"
      move_cursor(1, 0)
    else
      return false
    end
    true
  end

  def handle_edit_key(key)
    part = find_part_at(find_cursor_hole_index)
    case key
    when "ENTER", "a"
      start_placement
    when " "
      operate_part(part) if part
    when "+", "="
      adjust_part(part, 1) if part
    when "-"
      adjust_part(part, -1) if part
    when "x", "BS"
      remove_part(part) if part
    when "c"
      remove_all_parts if Widget.confirm(@sprite, "Clear board?")
    when "?"
      @help_visible = true
    when "q"
      @quit = true if Widget.confirm(@sprite, "Quit?")
    end
  end
end
