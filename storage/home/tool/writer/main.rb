class Writer
  def init_app
    init_doc
  end

  def save_doc
    doc_name = @doc_name.length > 0 ? @doc_name : prompt_doc_name
    return unless doc_name
    data = ""
    i = 0
    while i < @lines.length
      data += @lines[i] + "\n"
      i += 1
    end
    SD.write(doc_path(doc_name), data)
    @doc_name = doc_name
    @dirty = false
    @msg = "Saved " + doc_name
  rescue
    @msg = "Save failed"
  end

  def open_doc
    return if @dirty && !Widget.confirm(@sp, "Discard changes?")
    doc_name = pick_doc_name
    return unless doc_name
    lines = []
    SD.read(doc_path(doc_name)).split("\n").each do |line|
      lines.push(line.chomp)
    end
    lines.push("") if lines.length == 0
    @lines = lines
    @doc_name = doc_name
    @row = 0
    @col = 0
    @top = 0
    @top_segment_index = 0
    @dirty = false
    @msg = "Opened " + doc_name
    rebuild_line_segment_counts
    clamp_cursor
  rescue
    @msg = "Open failed"
  end

  def run
    init_app
    Display.fill_screen(Widget.theme_background)
    @sp = BandedSprite.new(12)
    begin
      SD.mount
      prepare_doc_storage
      while !@quit
        draw
        handle(read_key)
      end
    ensure
      SD.unmount
      @sp.delete if @sp
      Display.fill_screen(Widget.theme_background)
    end
  end
end

Writer.new.run
