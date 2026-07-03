class Writer
  def handle(input)
    code = input.length == 0 ? 0 : input.ord
    if input == "\e"
      @msg = HELP
    elsif input == "\e[D"
      move_left
    elsif input == "\e[C"
      move_right
    elsif input == "\e[A"
      move_up
    elsif input == "\e[B"
      move_down
    elsif code == 0
      toggle_input_mode
    elsif code == 17
      @quit = true
    elsif code == 11
      page_up
    elsif code == 10
      page_down
    elsif code == 19
      save_doc
    elsif code == 15
      open_doc
    elsif code == 13
      insert_newline
    elsif code == 8 || code == 127
      backspace
    elsif code == 28
      move_left
    elsif code == 29
      move_right
    elsif code == 30
      move_up
    elsif code == 31
      move_down
    elsif code >= 32 || input.length > 1
      insert_text(input)
    end
  end

  def toggle_input_mode
    @jp_mode = !@jp_mode
    @msg = @jp_mode ? "JP input" : "EN input"
  end

  def insert_text(text)
    line = current_line
    new_line = left_text(line, @col) + text + right_text(line, @col)
    set_current_line(new_line)
    @col += char_count(text)
    @dirty = true
    @msg = "Draft"
    clamp_cursor
  end

  def insert_newline
    line = current_line
    left = left_text(line, @col)
    right = right_text(line, @col)
    set_current_line(left)
    append_line(@row + 1, right)
    @row += 1
    @col = 0
    @dirty = true
    @msg = "Draft"
    clamp_cursor
  end

  def append_line(pos, text)
    @lines.push("")
    i = @lines.length - 1
    while i > pos
      @lines[i] = @lines[i - 1]
      i -= 1
    end
    @lines[pos] = text
  end

  def backspace
    changed = false
    if @col > 0
      line = current_line
      set_current_line(left_text(line, @col - 1) + right_text(line, @col))
      @col -= 1
      changed = true
    elsif @row > 0
      prev = @lines[@row - 1]
      prev = "" unless prev.is_a?(String)
      line = current_line
      @col = char_count(prev)
      @lines[@row - 1] = prev + line
      @lines.delete_at(@row)
      @row -= 1
      changed = true
    end
    @dirty = true if changed
    @msg = "Draft" if changed
    clamp_cursor
  end

  def move_left
    if @col > 0
      @col -= 1
    elsif @row > 0
      @row -= 1
      @col = char_count(current_line)
    end
    clamp_cursor
  end

  def move_right
    if @col < char_count(current_line)
      @col += 1
    elsif @row < @lines.length - 1
      @row += 1
      @col = 0
    end
    clamp_cursor
  end

  def move_up
    @row -= 1
    clamp_cursor
  end

  def move_down
    @row += 1
    clamp_cursor
  end

  def page_up
    @row -= visible_rows
    clamp_cursor
    @msg = "Scroll"
  end

  def page_down
    @row += visible_rows
    clamp_cursor
    @msg = "Scroll"
  end
end
