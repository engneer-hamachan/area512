# Block-yield sugar cannot be written in C: mruby/c invokes a proc by swapping
# the VM's current irep (c_proc_call), so a C function cannot yield and regain
# control for the ensure. Everything C-expressible lives in src/area512_fs.c.
class File
  def self.open(path, mode = "r")
    file = File.new(path, mode)
    return file unless block_given?

    begin
      yield file
    ensure
      file.close
    end
  end
end

class Dir
  def self.open(path)
    dir = Dir.new(path)
    return dir unless block_given?

    begin
      yield dir
    ensure
      dir.close
    end
  end
end
