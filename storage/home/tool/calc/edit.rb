class Calc
  def start_edit
    @editing = true
    @edit_buf = cell_at(@col, @row)
    @msg = "Editing " + selected_name
  end

  def commit_edit
    set_cell(@col, @row, @edit_buf)
    @editing = false
    @msg = selected_name + " updated"
  end

  def cancel_edit
    @editing = false
    @msg = "Canceled"
  end

  def handle_edit(input)
    code = 0
    code = input.ord if input.length > 0
    if code == 13 || code == 10
      commit_edit
    elsif code == 27
      cancel_edit
    elsif code == 8 || code == 127
      backspace_edit
    elsif code >= 32 && code < 127
      @edit_buf = @edit_buf + input
    end
  end

  def backspace_edit
    return if @edit_buf.length == 0
    cut = @edit_buf[0, @edit_buf.length - 1]
    @edit_buf = cut if cut.is_a?(String)
  end
end
