require 'area512-sprite'
require 'area512-sdfat'

class Calc
  def init_app
    init_sheet
  end

  def clear_cell
    set_cell(@col, @row, "")
    @msg = selected_name + " cleared"
  end

  def handle(input)
    if @editing
      handle_edit(input)
      return
    end
    case input
    when ","
      move(-1, 0)
    when "/"
      move(1, 0)
    when ";"
      move(0, -1)
    when "."
      move(0, 1)
    when "h"
      move(-1, 0)
    when "l"
      move(1, 0)
    when "k"
      move(0, -1)
    when "j"
      move(0, 1)
    when "e", "\r", "\n"
      start_edit
    when "c"
      clear_cell
    when "g"
      goto_cell
    when "s"
      save_file
    when "o"
      open_file
    when "q"
      @quit = true
    else
      @msg = HELP
    end
  end

  def save_file
    sheet_name = @sheet_name.length > 0 ? @sheet_name : prompt_sheet_name
    return unless sheet_name
    data = ""
    r = 0
    while r < ROWS
      c = 0
      while c < COLS
        v = cell_at(c, r)
        data += cell_name(c, r) + "\t" + v + "\n" if v.length > 0
        c += 1
      end
      r += 1
    end
    SD.write(sheet_path(sheet_name), data)
    @sheet_name = sheet_name
    @unsaved = false
    @msg = "Saved " + sheet_name
  rescue
    @msg = "Save failed"
  end

  def goto_cell
    name = read_entry("Goto cell: ", selected_name)
    if cell_ref?(name)
      pos = ref_pos(name)
      @col = pos[0]
      @row = pos[1]
      adjust_scroll
      @msg = "Moved to " + selected_name
    else
      @msg = "Bad cell"
    end
  end

  def open_file
    return if @unsaved && !Widget.confirm(@sp, "Discard changes?")
    sheet_name = pick_sheet_name
    return unless sheet_name
    sheet_lines = SD.read(sheet_path(sheet_name)).split("\n")
    init_sheet
    sheet_lines.each do |line|
      parts = line.chomp.split("\t", 2)
      if parts.length == 2 && cell_ref?(parts[0])
        pos = ref_pos(parts[0])
        set_cell(pos[0], pos[1], parts[1])
      end
    end
    @sheet_name = sheet_name
    @unsaved = false
    @msg = "Opened " + sheet_name
  rescue
    @msg = "Open failed"
  end

  def run
    init_app
    Display.fill_screen(C_BG)
    @sp = BandedSprite.new(12)
    begin
      SD.mount
      prepare_sheet_storage
      while !@quit
        draw
        handle(read_char)
      end
    ensure
      SD.unmount
      @sp.delete if @sp
      Display.fill_screen(C_BG)
    end
  end
end

Calc.new.run
