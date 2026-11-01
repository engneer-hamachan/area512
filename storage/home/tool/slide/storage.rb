class SlideShow
  def project_path(project_name)
    PROJECT_DIRECTORY + "/" + project_name
  end

  def read_entry_names(directory_path)
    entry_names = []

    directory = Dir.open(directory_path)
    begin
      while entry_name = directory.read
        entry_names.push(entry_name) unless entry_name == "." || entry_name == ".."
      end
    ensure
      directory.close if directory
    end
    entry_names
  end

  def numeric_markdown_paths(directory_path)
    markdown_paths = []

    read_entry_names(directory_path).each do |entry_name|
      next unless entry_name.end_with?(".md")

      number_text = entry_name.slice(0, entry_name.length - 3)
      next unless number_text.to_i.to_s == number_text

      markdown_paths.push(directory_path + "/" + entry_name)
    end

    markdown_paths.sort! do |left_markdown_path, right_markdown_path|
      left_markdown_path.split("/")[-1].to_i <=>
        right_markdown_path.split("/")[-1].to_i
    end
    markdown_paths
  end

  def create_project_directory
    Dir.mkdir(PROJECT_DIRECTORY) unless Dir.exist?(PROJECT_DIRECTORY)
  rescue
    @error_message = "Storage setup failed"
  end

  def list_project_names
    project_names = []

    read_entry_names(PROJECT_DIRECTORY).each do |entry_name|
      project_names.push(entry_name) if File.directory?(project_path(entry_name))
    end

    project_names.sort!
    project_names
  end

  def draw_project_list(project_list)
    while @sprite.draw
      @sprite.fill(Widget.theme_background)
      Widget.header(@sprite, "OPEN", PROJECT_DIRECTORY)
      project_list.draw(@sprite)
      Widget.hints(@sprite, [["Enter", "Open"], ["Esc", "Exit"]])
    end
  end

  def select_project_name(project_list, project_names)
    while true
      draw_project_list(project_list)
      key = read_key
      return nil if key == "ESC"
      return project_names[project_list.index] if key == "ENTER" && project_list.count > 0

      project_list.handle(key)
    end
  end

  def pick_project_name
    project_names = list_project_names
    project_list = WidgetList.new
    begin
      project_list.empty_text = "No projects"
      project_names.each do |project_name|
        project_list.add(project_name)
      end
      select_project_name(project_list, project_names)
    ensure
      project_list.delete
    end
  end

  def load_pages(selected_project_path)
    @pages = []
    @error_message = nil
    markdown_path = ""

    numeric_markdown_paths(selected_project_path).each do |markdown_path|
      markdown_content = SD.read(markdown_path)
      markdown_rows = parse_markdown_rows(markdown_content)
      @pages.push([markdown_path, markdown_rows])
    end

    if @pages.length == 0
      @error_message = "No numbered .md files"
    end
  rescue => error_message
    @error_message = error_message.message
  end

  def open_project
    return if @error_message

    project_name = pick_project_name
    if project_name
      load_pages(project_path(project_name))
    else
      @quit = true
    end
  rescue
    @error_message = "Open failed"
  end
end
