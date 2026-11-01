class Writer
  def doc_path(doc_name)
    DOC_DIRECTORY + "/" + doc_name
  end

  def doc_label
    @doc_name.length > 0 ? @doc_name : "untitled"
  end

  def create_doc_directory
    Dir.mkdir(DOC_DIRECTORY) unless Dir.exist?(DOC_DIRECTORY)
  end

  def move_legacy_doc
    return unless File.exist?(LEGACY_DOC_FILE)
    return if File.exist?(doc_path(LEGACY_DOC_NAME))
    File.rename(LEGACY_DOC_FILE, doc_path(LEGACY_DOC_NAME))
  end

  def prepare_doc_storage
    create_doc_directory
    move_legacy_doc
  rescue
    @msg = "Storage setup failed"
  end

  def list_doc_names
    doc_names = []
    directory = Dir.open(DOC_DIRECTORY)
    begin
      while entry_name = directory.read
        doc_names.push(entry_name) if entry_name.end_with?(DOC_EXTENSION)
      end
    ensure
      directory.close
    end
    doc_names.sort!
    doc_names
  end

  def read_list_key
    key = ""
    STDIN.raw do
      key = Widget.read_key
    end
    key
  end

  def draw_doc_list(doc_list)
    while @sp.draw
      @sp.fill(Widget.theme_background)
      Widget.header(@sp, "OPEN", DOC_DIRECTORY)
      doc_list.draw(@sp)
      Widget.hints(@sp, [["Enter", "Open"], ["Esc", "Cancel"]])
    end
  end

  def select_doc_name(doc_list, doc_names)
    while true
      draw_doc_list(doc_list)
      key = read_list_key
      return nil if key == "ESC"
      return doc_names[doc_list.index] if key == "ENTER" && doc_list.count > 0
      doc_list.handle(key)
    end
  end

  def pick_doc_name
    doc_names = list_doc_names
    doc_list = WidgetList.new
    begin
      doc_list.empty_text = "No files"
      i = 0
      while i < doc_names.length
        doc_list.add(doc_names[i])
        i += 1
      end
      select_doc_name(doc_list, doc_names)
    ensure
      doc_list.delete
    end
  end

  def prompt_doc_name
    typed_name = Widget.input(@sp, "FILE:")
    return nil if typed_name.nil? || typed_name.length == 0
    if typed_name.include?("/")
      @msg = "Bad file name"
      return nil
    end
    doc_name = typed_name
    doc_name += DOC_EXTENSION unless doc_name.end_with?(DOC_EXTENSION)
    return doc_name unless File.exist?(doc_path(doc_name))
    Widget.confirm(@sp, "Overwrite?") ? doc_name : nil
  end
end
