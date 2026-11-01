class Calc
  def sheet_path(sheet_name)
    SHEET_DIRECTORY + "/" + sheet_name
  end

  def create_sheet_directory
    Dir.mkdir(SHEET_DIRECTORY) unless Dir.exist?(SHEET_DIRECTORY)
  end

  def move_legacy_sheet
    return unless File.exist?(LEGACY_SHEET_FILE)
    return if File.exist?(sheet_path(LEGACY_SHEET_NAME))
    File.rename(LEGACY_SHEET_FILE, sheet_path(LEGACY_SHEET_NAME))
  end

  def prepare_sheet_storage
    create_sheet_directory
    move_legacy_sheet
  rescue
    @msg = "Storage setup failed"
  end

  def list_sheet_names
    sheet_names = []
    directory = Dir.open(SHEET_DIRECTORY)
    begin
      while entry_name = directory.read
        sheet_names.push(entry_name) if entry_name.end_with?(SHEET_EXTENSION)
      end
    ensure
      directory.close
    end
    sheet_names.sort!
    sheet_names
  end

  def read_list_key
    key = ""
    STDIN.raw do
      key = Widget.read_key
    end
    key
  end

  def draw_sheet_list(sheet_list)
    while @sp.draw
      @sp.fill(C_BG)
      Widget.header(@sp, "OPEN", SHEET_DIRECTORY)
      sheet_list.draw(@sp)
      Widget.hints(@sp, [["Enter", "Open"], ["Esc", "Cancel"]])
    end
  end

  def select_sheet_name(sheet_list, sheet_names)
    while true
      draw_sheet_list(sheet_list)
      key = read_list_key
      return nil if key == "ESC"
      if key == "ENTER" && sheet_list.count > 0
        return sheet_names[sheet_list.index]
      end
      sheet_list.handle(key)
    end
  end

  def pick_sheet_name
    sheet_names = list_sheet_names
    sheet_list = WidgetList.new
    begin
      sheet_list.empty_text = "No files"
      i = 0
      while i < sheet_names.length
        sheet_list.add(sheet_names[i])
        i += 1
      end
      select_sheet_name(sheet_list, sheet_names)
    ensure
      sheet_list.delete
    end
  end

  def prompt_sheet_name
    typed_name = Widget.input(@sp, "FILE:")
    return nil if typed_name.nil? || typed_name.length == 0
    if typed_name.include?("/")
      @msg = "Bad file name"
      return nil
    end
    sheet_name = typed_name
    unless sheet_name.end_with?(SHEET_EXTENSION)
      sheet_name += SHEET_EXTENSION
    end
    return sheet_name unless File.exist?(sheet_path(sheet_name))
    Widget.confirm(@sp, "Overwrite?") ? sheet_name : nil
  end
end
