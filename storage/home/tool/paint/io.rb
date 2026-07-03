require 'io/console'

def read_key
  key = nil
  STDIN.raw do
    key = STDIN.getch
  end
  key = "" if key.nil?
  key
end
