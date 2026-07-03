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
    SD.write(SHEET_FILE, data)
    @msg = "Saved to " + SHEET_FILE
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
    init_sheet
    SD.read(SHEET_FILE).split("\n").each do |line|
      parts = line.chomp.split("\t", 2)
      if parts.length == 2 && cell_ref?(parts[0])
        pos = ref_pos(parts[0])
        set_cell(pos[0], pos[1], parts[1])
      end
    end
    @msg = "Opened " + SHEET_FILE
  rescue
    @msg = "Open failed"
  end

  def run
    init_app
    Display.fill_screen(C_BG)
    @sp = Sprite.new(Display.width, Display.height, 12)
    begin
      SD.mount
      SD.mkdir(DATA_DIR)
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
