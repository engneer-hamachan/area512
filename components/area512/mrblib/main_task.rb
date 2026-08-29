require "machine"
require "watchdog"
Watchdog.disable
require "env"
require "io/console"
require "sandbox"
require "area512-sandbox"
require "area512-compile"
require "area512-micropython"
require "area512-console"
require "vim"

begin
  require "area512-sprite"
rescue LoadError
end

require "area512-dot"

# The filer UI (rendering, input loop) lives in C (picoruby-area512-filer) to
# keep per-frame churn off the mruby/c heap; this file is a thin dispatch shell.
require "area512-filer"
# All files live on the SD card; area512-sdfat provides SD plus the minimal
# File/Dir used by this file, require.rb and Sandbox. Ruby-visible paths are
# rooted at the card's Area512_data directory ("/" here == that directory).
require "area512-sdfat"

STDIN = IO.new unless Object.const_defined?(:STDIN)
STDOUT = IO.new unless Object.const_defined?(:STDOUT)
ARGV = [] unless Object.const_defined?(:ARGV)

# Must match filer.c.
T_UP = 0 unless Object.const_defined?(:T_UP)
T_DIR = 1 unless Object.const_defined?(:T_DIR)
T_FILE = 2 unless Object.const_defined?(:T_FILE)
ACT_QUIT = 0 unless Object.const_defined?(:ACT_QUIT)
ACT_OPEN_DIR = 1 unless Object.const_defined?(:ACT_OPEN_DIR)
ACT_UP = 2 unless Object.const_defined?(:ACT_UP)
ACT_RUN_RUBY = 3 unless Object.const_defined?(:ACT_RUN_RUBY)
ACT_COMPILE = 4 unless Object.const_defined?(:ACT_COMPILE)
ACT_COMPILE_ALL = 5 unless Object.const_defined?(:ACT_COMPILE_ALL)
ACT_RUN_DIR = 6 unless Object.const_defined?(:ACT_RUN_DIR)
ACT_EDIT = 7 unless Object.const_defined?(:ACT_EDIT)
ACT_NEW_FILE = 8 unless Object.const_defined?(:ACT_NEW_FILE)
ACT_NEW_DIR = 9 unless Object.const_defined?(:ACT_NEW_DIR)
ACT_DELETE = 10 unless Object.const_defined?(:ACT_DELETE)
ACT_REBOOT = 11 unless Object.const_defined?(:ACT_REBOOT)
ACT_MOVE = 12 unless Object.const_defined?(:ACT_MOVE)
ACT_VIEW_MARKDOWN = 13 unless Object.const_defined?(:ACT_VIEW_MARKDOWN)
ACT_RUN_PYTHON = 14 unless Object.const_defined?(:ACT_RUN_PYTHON)
ACT_COPY = 15 unless Object.const_defined?(:ACT_COPY)
ACT_EDIT_DOT = 16 unless Object.const_defined?(:ACT_EDIT_DOT)
ACT_CHANGE_DIR = 17 unless Object.const_defined?(:ACT_CHANGE_DIR)

def run_sd_error_screen(filer)
  filer.cwd = "/"
  filer.clear_entries
  filer.add_entry("Retry SD mount", T_UP)
  filer.message = "No SD card: Enter = retry, r = reboot"

  while true
    act = filer.run
    Machine.reboot 0 if act == ACT_REBOOT

    begin
      SD.mount
      return
    rescue => e
      filer.message = "#{e.message}: Enter = retry, r = reboot"
    end
  end
end

def setup_environment
  $LOAD_PATH = ["/lib"]
  ENV["HOME"] = "/home"
  ENV["PATH"] = "/bin"
end

# --- Path/name helpers (File lacks basename/dirname in this build) ---

def join_path(dir, name)
  dir == "/" ? "/#{name}" : "#{dir}/#{name}"
end

def find_last_slash_index(path)
  last_slash_index = -1

  i = 0
  while i < path.length
    last_slash_index = i if path[i, 1] == "/"
    i += 1
  end

  last_slash_index
end

def base_name(path)
  last_slash_index = find_last_slash_index(path)

  return path if last_slash_index < 0

  path[last_slash_index + 1, path.length - last_slash_index - 1]
end

def dir_name(path)
  last_slash_index = find_last_slash_index(path)

  return "/" if last_slash_index <= 0

  path[0, last_slash_index]
end

def entry_type_at(path)
  File.directory?(path) ? T_DIR : T_FILE
end

def strip_extension(name)
  if name.end_with?(".mrb") || name.end_with?(".mpy")
    name[0, name.length - 4]
  elsif name.end_with?(".rb") || name.end_with?(".py")
    name[0, name.length - 3]
  else
    name
  end
end

def ruby_bytecode_name(name)
  name.end_with?(".rb") ? "#{strip_extension(name)}.mrb" : name
end

def python_bytecode_name(name)
  name.end_with?(".py") ? "#{strip_extension(name)}.mpy" : name
end

# --- Build the entry list (fed to C) ---

def entry_names(cwd)
  names = []

  dir = Dir.open(cwd)
  begin
    while name = dir.read
      names << name unless name == "." || name == ".."
    end
  ensure
    dir.close if dir
  end

  names
end

def build_entries(filer, cwd)
  dirs = []
  files = []

  names = entry_names(cwd)

  i = 0
  while i < names.length
    name = names[i]

    if File.directory?(join_path(cwd, name))
      dirs << name
    else
      files << name
    end

    i += 1
  end

  filer.clear_entries
  filer.add_entry("..", T_UP) unless cwd == "/"

  i = 0
  while i < dirs.length
    filer.add_entry(dirs[i], T_DIR)
    i += 1
  end

  i = 0
  while i < files.length
    filer.add_entry(files[i], T_FILE)
    i += 1
  end
end

# --- Compile/run (Sandbox) and edit (Vim) ---
# These return a status string for the filer status line, not terminal output.

def compile_source(dir, name)
  source_path = join_path(dir, name)

  begin
    if name.end_with?(".py")
      bytecode_path = join_path(dir, python_bytecode_name(name))
      Console.reset

      begin
        MicroPython.compile_file(source_path, bytecode_path)
      ensure
        Console.wait_key_if_output
      end
    else
      bytecode_path = join_path(dir, ruby_bytecode_name(name))
      Area512.compile_file(source_path, bytecode_path)
    end

    nil
  rescue => e
    "#{e.class}: #{e.message}"
  end
end

def compile_entry(dir, name)
  error_message = compile_source(dir, name)
  error_message ? error_message : "Compiled #{name}"
end

def compile_all(dir)
  names = entry_names(dir)
  compiled_count = 0
  error_message = nil

  i = 0
  while i < names.length
    name = names[i]

    if name.end_with?(".rb") || name.end_with?(".py")
      error_message = compile_source(dir, name)
      break if error_message

      compiled_count += 1
    end

    i += 1
  end

  return error_message if error_message
  return "No source here" if compiled_count == 0

  "Compiled #{compiled_count} file(s)"
end

def run_mrb(path, label = strip_extension(base_name(path)))
  Console.reset
  sandbox = Sandbox.new(label)

  result =
    begin
      sandbox.load_file(path)
      exception_message = sandbox.area512_exception_message

      if exception_message
        puts "Error: #{label}: #{exception_message}"
        "Error: #{label}"
      else
        "Returned: #{label}"
      end

    rescue => e
      puts "Error: #{label}: #{e.message} (#{e.class})"
      "Error: #{label}"
    ensure
      sandbox.cleanup
      sandbox = nil
    end

  Console.wait_key_if_output
  result
end

def run_manifest(dir, manifest_path, label)
  Console.reset
  sandbox = Sandbox.new(label)

  result =
    begin
      sandbox.load_manifest(dir, manifest_path)
      exception_message = sandbox.area512_exception_message

      if exception_message
        puts "Error: #{label}: #{exception_message}"
        "Error: #{label}"
      else
        "Returned: #{label}"
      end

    rescue => e
      puts "Error: #{label}: #{e.message} (#{e.class})"
      "Error: #{label}"
    ensure
      sandbox.cleanup
      sandbox = nil
    end

  Console.wait_key_if_output
  result
end

def run_mpy(
  python_bytecode_path,
  label = strip_extension(base_name(python_bytecode_path))
)
  Console.reset

  run_message =
    begin
      MicroPython.run_bytecode_file(python_bytecode_path)
      "Returned: #{label}"
    rescue
      "Error: #{label}"
    end

  Console.wait_key_if_output

  run_message
end

def run_python_manifest(dir, manifest_path, label)
  Console.reset

  run_message =
    begin
      MicroPython.run_manifest(dir, manifest_path)
      "Returned: #{label}"
    rescue
      "Error: #{label}"
    end

  Console.wait_key_if_output

  run_message
end

def run_ruby(dir, name)
  error_message = name.end_with?(".rb") ? compile_source(dir, name) : nil
  return error_message if error_message

  run_mrb(join_path(dir, ruby_bytecode_name(name)))
end

def run_python(dir, name)
  error_message = name.end_with?(".py") ? compile_source(dir, name) : nil
  return error_message if error_message

  run_mpy(join_path(dir, python_bytecode_name(name)))
end

def show_app_image(dir)
  image_path = join_path(dir, "image.h")
  return unless File.exist?(image_path)

  begin
    Display.show_header_image(image_path, 3000)
  rescue
    # Splash image is optional; keep launching the app even if it fails.
  end
end

def python_manifest?(manifest_path)
  file = File.open(manifest_path, "r")

  begin
    text = file.read
  ensure
    file.close
  end

  return false unless text

  lines = text.split("\n")

  i = 0
  while i < lines.length
    line = lines[i].strip
    i += 1

    next if line.empty? || line.start_with?("#")

    return line.end_with?(".mpy")
  end

  false
end

def run_dir(dir, name)
  app_dir = join_path(dir, name)

  manifest_path = join_path(app_dir, "main.manifest")
  ruby_main_path = join_path(app_dir, "main.mrb")
  python_main_path = join_path(app_dir, "main.mpy")

  if File.exist?(manifest_path)
    show_app_image(app_dir)

    if python_manifest?(manifest_path)
      run_python_manifest(app_dir, manifest_path, name)
    else
      run_manifest(app_dir, manifest_path, name)
    end

  elsif File.exist?(ruby_main_path)
    show_app_image(app_dir)
    run_mrb(ruby_main_path, name)

  elsif File.exist?(python_main_path)
    show_app_image(app_dir)
    run_mpy(python_main_path, name)

  else
    "No main.manifest, main.mrb or main.mpy in #{name}"
  end
end

def view_markdown(dir, name)
  begin
    Markdown.new(join_path(dir, name)).show
    "Returned from viewer"
  rescue => e
    "#{e.class}: #{e.message}"
  end
end

def edit_dot(dir, name)
  begin
    Dot.edit(join_path(dir, name))
    "Returned from dot"
  rescue => e
    "#{e.class}: #{e.message}"
  end
end

def edit_entry(dir, name)
  begin
    Vim.new(join_path(dir, name)).start
    "Returned from vim"
  rescue => e
    "#{e.class}: #{e.message}"
  end
end

# --- Create / delete ---

def create_file(cwd, name)
  return "Empty name" if name.nil? || name.empty?

  path = join_path(cwd, name)
  return "Already exists" if File.exist?(path)

  begin
    File.open(path, "w") { |f| }
    "Created #{name}"
  rescue => e
    "#{e.class}: #{e.message}"
  end
end

def create_dir(cwd, name)
  return "Empty name" if name.nil? || name.empty?

  path = join_path(cwd, name)
  return "Already exists" if File.exist?(path)

  begin
    Dir.mkdir(path)
    "Created #{name}/"
  rescue => e
    "#{e.class}: #{e.message}"
  end
end

# Dir.rmdir only removes empty dirs.
def remove_tree(path)
  if File.directory?(path)
    names = entry_names(path)

    i = 0
    while i < names.length
      remove_tree(join_path(path, names[i]))
      i += 1
    end

    Dir.rmdir(path)
  else
    File.unlink(path)
  end
end

def delete_entry(dir, name, entry_type)
  begin
    if entry_type == T_DIR
      remove_tree(join_path(dir, name))
    else
      File.unlink(join_path(dir, name))
    end

    "Deleted #{name}"
  rescue => e
    "#{e.class}: #{e.message}"
  end
end

# --- Move (m, destination typed relative to cwd or absolute) ---

# Collapses "." and ".." components; nil when ".." would climb above the root
# (the Ruby-side half of the path traversal guard, so typed ".." still works).
def normalize_path(path)
  parts = path.split("/")
  kept = []

  i = 0
  while i < parts.length
    part = parts[i]
    i += 1

    next if part == "" || part == "."

    if part == ".."
      return nil if kept.empty?
      kept.pop
    else
      kept << part
    end
  end

  result = "/"

  i = 0
  while i < kept.length
    result = join_path(result, kept[i])
    i += 1
  end

  result
end

def normalize_input_path(cwd, input)
  normalize_path(input.start_with?("/") ? input : join_path(cwd, input))
end

def move_entry(cwd, source_path, entry_type, destination_input)
  return "Empty path" if destination_input.nil? || destination_input.empty?

  name = base_name(source_path)
  normalized_input_path = normalize_input_path(cwd, destination_input)

  return "Bad path" unless normalized_input_path

  destination_path =
    if File.directory?(normalized_input_path)
      join_path(normalized_input_path, name)
    else
      normalized_input_path
    end

  return "No such directory" unless File.directory?(dir_name(destination_path))
  return "Same path" if destination_path == source_path

  if entry_type == T_DIR && destination_path.start_with?("#{source_path}/")
    return "Can't move into itself"
  end

  if File.exist?(destination_path)
    return "Already exists: #{base_name(destination_path)}"
  end

  begin
    File.rename(source_path, destination_path)

    "Moved #{name} -> #{destination_path}"
  rescue => e
    "#{e.class}: #{e.message}"
  end
end

# --- Copy (C, destination typed relative to cwd or absolute) ---

# There is no recursive copy in the fs API; SD.read loads a whole file into the
# mruby/c heap, so a file larger than the free heap fails with "No memory".
def copy_tree(source_path, destination_path)
  if File.directory?(source_path)
    Dir.mkdir(destination_path)
    names = entry_names(source_path)

    index = 0
    while index < names.length
      copy_tree(
        join_path(source_path, names[index]),
        join_path(destination_path, names[index])
      )

      index += 1
    end
  else
    SD.write(destination_path, SD.read(source_path))
  end
end

def copy_entry(cwd, source_path, entry_type, destination_input)
  return "Empty path" if destination_input.nil? || destination_input.empty?

  name = base_name(source_path)
  normalized_input_path = normalize_input_path(cwd, destination_input)

  return "Bad path" unless normalized_input_path

  destination_path =
    if File.directory?(normalized_input_path)
      join_path(normalized_input_path, name)
    else
      normalized_input_path
    end

  return "No such directory" unless File.directory?(dir_name(destination_path))
  return "Same path" if destination_path == source_path

  if entry_type == T_DIR && destination_path.start_with?("#{source_path}/")
    return "Can't copy into itself"
  end

  if File.exist?(destination_path)
    return "Already exists: #{base_name(destination_path)}"
  end

  begin
    copy_tree(source_path, destination_path)

    "Copied #{name} -> #{destination_path}"
  rescue => e
    "#{e.class}: #{e.message}"
  end
end

# --- Dispatch shell ---

def run_filer(filer, root)
  cwd = root
  filer.cwd = cwd
  build_entries(filer, cwd)

  while true
    act = filer.run
    msg = ""

    action_target_path = filer.action_target_path

    target_path =
      if action_target_path.empty?
        join_path(cwd, filer.selected_name)
      else
        action_target_path
      end

    target_parent_dir = dir_name(target_path)
    target_name = base_name(target_path)

    case act
    when ACT_QUIT
      break

    when ACT_OPEN_DIR
      cwd = join_path(cwd, filer.selected_name)
      filer.index = 0

    when ACT_UP
      cwd = dir_name(cwd)
      filer.index = 0

    when ACT_CHANGE_DIR
      path = normalize_input_path(cwd, filer.input_text)

      if path.nil?
        msg = "Bad path"
      elsif !File.directory?(path)
        msg = "No such directory"
      else
        cwd = path
        filer.index = 0
      end

    when ACT_RUN_RUBY
      msg = run_ruby(target_parent_dir, target_name)

    when ACT_COMPILE
      msg = compile_entry(target_parent_dir, target_name)

    when ACT_COMPILE_ALL
      msg = compile_all(action_target_path.empty? ? cwd : target_path)

    when ACT_RUN_DIR
      msg = run_dir(target_parent_dir, target_name)

    when ACT_VIEW_MARKDOWN
      msg = view_markdown(target_parent_dir, target_name)

    when ACT_RUN_PYTHON
      msg = run_python(target_parent_dir, target_name)

    when ACT_EDIT
      msg = edit_entry(target_parent_dir, target_name)

    when ACT_EDIT_DOT
      msg = edit_dot(target_parent_dir, target_name)

    when ACT_NEW_FILE
      msg = create_file(cwd, filer.input_text)

    when ACT_NEW_DIR
      msg = create_dir(cwd, filer.input_text)

    when ACT_DELETE
      msg =
        delete_entry(
          target_parent_dir,
          target_name,
          entry_type_at(target_path)
        )

    when ACT_MOVE
      msg =
        move_entry(
          cwd,
          target_path,
          entry_type_at(target_path),
          filer.input_text
        )

    when ACT_COPY
      msg =
        copy_entry(
          cwd,
          target_path,
          entry_type_at(target_path),
          filer.input_text
        )

    when ACT_REBOOT
      Machine.reboot 0
    end

    filer.cwd = cwd
    build_entries(filer, cwd)
    filer.message = msg
  end
end

begin
  STDIN.echo = false

  filer = Filer.new

  begin
    SD.mount
  rescue
    run_sd_error_screen(filer)
  end

  SD.restore_seed
  setup_environment

  run_filer(filer, ENV["HOME"])

rescue => e
  puts "#{e.message} (#{e.class})"
rescue Exception => e
  puts "FATAL: #{e.message} (#{e.class})"
end
