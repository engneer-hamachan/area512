require 'io/console'

def read_char
  input = nil
  STDIN.raw do
    input = STDIN.getch
  end
  input = "" if input.nil?
  input
end

def read_entry(prompt, initial)
  print prompt
  print initial
  buf = initial
  loop do
    ch = read_char
    next if ch.length == 0
    code = ch.ord
    if code == 13 || code == 10
      puts
      return buf
    elsif code == 27
      puts
      return initial
    elsif code == 8 || code == 127
      if buf.length > 0
        cut = buf[0, buf.length - 1]
        buf = cut if cut.is_a?(String)
        print "\b \b"
      end
    elsif code >= 32 && code < 127
      buf = buf + ch
      print ch
    end
  end
end
