require "machine"
require "watchdog"
Watchdog.disable
require "env"
require "io/console"
require "littlefs"
require "vfs"
require "sandbox"
require "area512-sandbox"
require "area512-compile"
require "vim"

begin
  require "area512-sprite"
rescue LoadError
end

# The filer UI (rendering, input loop) lives in C (picoruby-area512-filer) to
# keep per-frame churn off the mruby/c heap; this file is a thin dispatch shell.
require "area512-filer"
# SD is not required at boot; apps that need it require "area512-sdfat" + SD.mount.

STDIN = IO.new unless Object.const_defined?(:STDIN)
STDOUT = IO.new unless Object.const_defined?(:STDOUT)
ARGV = [] unless Object.const_defined?(:ARGV)

# Must match filer.c.
T_UP = 0 unless Object.const_defined?(:T_UP)
T_DIR = 1 unless Object.const_defined?(:T_DIR)
T_APP = 2 unless Object.const_defined?(:T_APP)
T_OTHER = 3 unless Object.const_defined?(:T_OTHER)
ACT_QUIT = 0 unless Object.const_defined?(:ACT_QUIT)
ACT_OPEN_DIR = 1 unless Object.const_defined?(:ACT_OPEN_DIR)
ACT_UP = 2 unless Object.const_defined?(:ACT_UP)
ACT_RUN_MRB = 3 unless Object.const_defined?(:ACT_RUN_MRB)
ACT_COMPILE = 4 unless Object.const_defined?(:ACT_COMPILE)
ACT_COMPILE_ALL = 5 unless Object.const_defined?(:ACT_COMPILE_ALL)
ACT_RUN_DIR = 6 unless Object.const_defined?(:ACT_RUN_DIR)
ACT_EDIT = 7 unless Object.const_defined?(:ACT_EDIT)
ACT_NEW_FILE = 8 unless Object.const_defined?(:ACT_NEW_FILE)
ACT_NEW_DIR = 9 unless Object.const_defined?(:ACT_NEW_DIR)
ACT_DELETE = 10 unless Object.const_defined?(:ACT_DELETE)
ACT_REBOOT = 11 unless Object.const_defined?(:ACT_REBOOT)

def setup_root_volume(device, label: "storage")
  return if VFS.volume_index("/")

  fs = Littlefs.new(device, label: label)
  retry_count = 0

  begin
    VFS.mount(fs, "/")
  rescue => e
    fs.mkfs

    retry_count += 1
    retry if retry_count == 1

    raise e
  end
end

def create_base_dirs(root = "")
  Dir.chdir(root.empty? ? "/" : root) do
    dirs = %w(home lib bin)

    i = 0
    while i < dirs.size
      dir = dirs[i]
      Dir.mkdir(dir) unless Dir.exist?(dir)
      i += 1
    end
  end
end

def setup_environment(root = "")
  $LOAD_PATH = ["#{root}/lib"]
  ENV["HOME"] = "#{root}/home"
  ENV["PATH"] = "#{root}/bin"
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

def strip_extension(name)
  if name.end_with?(".mrb")
    name[0, name.length - 4]
  elsif name.end_with?(".rb")
    name[0, name.length - 3]
  else
    name
  end
end

# --- Build the entry list (fed to C) ---

def add_app(apps, base, has_rb, has_mrb)
  i = 0
  while i < apps.length
    if apps[i][0] == base
      apps[i][1] = true if has_rb
      apps[i][2] = true if has_mrb
      return
    end

    i += 1
  end

  apps << [base, has_rb, has_mrb]
end

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
  apps = []
  others = []

  names = entry_names(cwd)

  i = 0
  while i < names.length
    name = names[i]
    full = join_path(cwd, name)

    if File.directory?(full)
      dirs << name
    elsif name.end_with?(".mrb")
      add_app(apps, strip_extension(name), false, true)
    elsif name.end_with?(".rb")
      add_app(apps, strip_extension(name), true, false)
    else
      others << name
    end

    i += 1
  end

  filer.clear_entries
  filer.add_entry("..", T_UP, false, false) unless cwd == "/"

  if cwd == "/"
    vols = VFS::VOLUMES

    vi = 0
    while vi < vols.length
      mountpoint = vols[vi][:mountpoint]

      if mountpoint != "/" && mountpoint.start_with?("/")
        filer.add_entry(mountpoint[1, mountpoint.length - 1], T_DIR, false, false)
      end

      vi += 1
    end
  end

  i = 0
  while i < dirs.length
    filer.add_entry(dirs[i], T_DIR, false, false)
    i += 1
  end

  i = 0
  while i < apps.length
    app = apps[i]
    filer.add_entry(app[0], T_APP, app[1], app[2])
    i += 1
  end

  i = 0
  while i < others.length
    filer.add_entry(others[i], T_OTHER, false, false)
    i += 1
  end
end

# --- Compile/run (Sandbox) and edit (Vim) ---
# These return a status string for the filer status line, not terminal output.

def compile(src)
  dst = src[0, src.length - 3] + ".mrb"

  begin
    Area512.compile_file(src, dst)
    nil
  rescue => e
    "#{e.class}: #{e.message}"
  end
end

def compile_one(cwd, base)
  err = compile(join_path(cwd, "#{base}.rb"))
  err ? err : "Compiled #{base}"
end

def compile_all(cwd)
  bases = []
  names = entry_names(cwd)

  i = 0
  while i < names.length
    name = names[i]
    if name.end_with?(".rb")
      base = strip_extension(name)
      bases << base unless bases.include?(base)
    end
    i += 1
  end

  return "No .rb here" if bases.empty?

  compiled = 0
  error = nil

  j = 0
  while j < bases.length
    err = compile(join_path(cwd, "#{bases[j]}.rb"))
    if err
      error = "#{bases[j]}: #{err}"
      break
    end
    compiled += 1
    j += 1
  end

  error ? error : "Compiled #{compiled} file(s)"
end

def run_mrb(path, label = nil)
  label ||= strip_extension(base_name(path))
  sandbox = Sandbox.new(label)

  begin
    sandbox.load_file(path)

    if err = sandbox.error
      "#{err.class}: #{err.message}"
    else
      "Returned: #{label}"
    end

  rescue => e
    "#{e.class}: #{e.message}"
  ensure
    sandbox.cleanup if sandbox
  end
end

def run_manifest(dir, manifest_path, label)
  sandbox = Sandbox.new(label)

  begin
    sandbox.load_manifest(dir, manifest_path)

    if err = sandbox.error
      "#{err.class}: #{err.message}"
    else
      "Returned: #{label}"
    end

  rescue => e
    "#{e.class}: #{e.message}"
  ensure
    sandbox.cleanup if sandbox
  end
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

def run_selected_dir(cwd, name)
  dir = join_path(cwd, name)

  manifest_path = join_path(dir, "main.manifest")
  main_path = join_path(dir, "main.mrb")

  if File.exist?(manifest_path)
    show_app_image(dir)
    run_manifest(dir, manifest_path, name)
  elsif File.exist?(main_path)
    show_app_image(dir)
    run_mrb(main_path, name)
  else
    "No main.manifest or main.mrb in #{name}"
  end
end

def edit_entry(cwd, name, type)
  src = (type == T_APP) ? join_path(cwd, "#{name}.rb") : join_path(cwd, name)

  begin
    Vim.new(src).start
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

def delete_entry(cwd, name, type, has_rb, has_mrb)
  begin
    # Don't delete a mount point (e.g. /sd) or remove_tree would wipe the SD.
    if cwd == "/" && VFS.volume_index("/#{name}")
      return "Can't delete a mount point"
    end

    if type == T_DIR
      remove_tree(join_path(cwd, name))
    elsif type == T_APP
      File.unlink(join_path(cwd, "#{name}.rb"))  if has_rb
      File.unlink(join_path(cwd, "#{name}.mrb")) if has_mrb
    else
      File.unlink(join_path(cwd, name))
    end

    "Deleted #{name}"
  rescue => e
    "#{e.class}: #{e.message}"
  end
end

# --- Dispatch shell ---

def run_filer(root)
  filer = Filer.new
  cwd = root
  filer.cwd = cwd
  build_entries(filer, cwd)

  while true
    act = filer.run
    msg = ""

    case act
    when ACT_QUIT
      break

    when ACT_OPEN_DIR
      cwd = join_path(cwd, filer.selected_name)
      filer.index = 0

    when ACT_UP
      cwd = dir_name(cwd)
      cwd = "/" if cwd.empty?
      filer.index = 0

    when ACT_RUN_MRB
      msg = run_mrb(join_path(cwd, "#{filer.selected_name}.mrb"))

    when ACT_COMPILE
      msg = compile_one(cwd, filer.selected_name)

    when ACT_COMPILE_ALL
      msg = compile_all(cwd)

    when ACT_RUN_DIR
      msg = run_selected_dir(cwd, filer.selected_name)

    when ACT_EDIT
      msg = edit_entry(cwd, filer.selected_name, filer.selected_type)

    when ACT_NEW_FILE
      msg = create_file(cwd, filer.input_text)

    when ACT_NEW_DIR
      msg = create_dir(cwd, filer.input_text)

    when ACT_DELETE
      msg =
        delete_entry(
          cwd,
          filer.selected_name,
          filer.selected_type,
          filer.selected_rb,
          filer.selected_mrb
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

  setup_root_volume(:flash, label: "storage")
  create_base_dirs
  setup_environment

  Dir.chdir ENV["HOME"]

  run_filer(ENV["HOME"])

rescue => e
  puts "#{e.message} (#{e.class})"
rescue Exception => e
  puts "FATAL: #{e.message} (#{e.class})"
end
