class Writer
  def init_app
    init_doc
  end

  def save_doc
    data = ""
    i = 0
    while i < @lines.length
      data += @lines[i] + "\n"
      i += 1
    end
    SD.write(DOC_FILE, data)
    @dirty = false
    @msg = "Saved " + DOC_NAME
  rescue
    @msg = "Save failed"
  end

  def open_doc
    @lines = []
    SD.read(DOC_FILE).split("\n").each do |line|
      @lines.push(line.chomp)
    end
    @lines.push("") if @lines.length == 0
    @row = 0
    @col = 0
    @top = 0
    @dirty = false
    @msg = "Opened " + DOC_NAME
  rescue
    @lines = [""] if @lines.length == 0
    @msg = "Open failed"
  ensure
    clamp_cursor
  end

  def run
    init_app
    Display.fill_screen(C_APP)
    @sp = Sprite.new(Display.width, Display.height, 12)
    begin
      SD.mount
      SD.mkdir(DATA_DIR)
      while !@quit
        draw
        handle(read_key)
      end
    ensure
      SD.unmount
      @sp.delete if @sp
      Display.fill_screen(C_APP)
    end
  end
end

Writer.new.run
