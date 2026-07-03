require 'io/console'
require 'area512-sdfat'

def read_key
  input = nil
  STDIN.raw do
    input = STDIN.getch
    if input == "\e"
      a = STDIN.getch
      b = STDIN.getch
      input = input + a + b if a && b
    end
  end
  input = "" if input.nil?
  input
end

def safe_mount
  SD.mount
rescue
end

def safe_unmount
  SD.unmount
rescue
end
