require 'io/console'
require 'area512-sprite'
require 'area512-sdfat'

class Scheduler
  W = 240 unless const_defined?(:W)
  H = 135 unless const_defined?(:H)
  SAVE_FILE = "/data/scheduler.txt" unless const_defined?(:SAVE_FILE)

  C_BG = 0x000000 unless const_defined?(:C_BG)
  C_AMBER = 0xF5972D unless const_defined?(:C_AMBER)
  C_TEXT = 0xCFA45F unless const_defined?(:C_TEXT)
  C_GOLD = 0xFFD966 unless const_defined?(:C_GOLD)
  C_DARK = 0x241604 unless const_defined?(:C_DARK)

  MONTHS = ["JAN", "FEB", "MAR", "APR", "MAY", "JUN",
            "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"] unless const_defined?(:MONTHS)
  WDAYS = ["SU", "MO", "TU", "WE", "TH", "FR", "SA"] unless const_defined?(:WDAYS)
end
