# Solitair -- terminal input helper (single raw keypress)

require 'io/console'

def read_char
  input = nil
  STDIN.raw do
    input = STDIN.getch
  end
  input = "" if input.nil?
  input
end
