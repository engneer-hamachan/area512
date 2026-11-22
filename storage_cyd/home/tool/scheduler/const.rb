require 'area512-widget'
require 'machine'
require 'io/console'
require 'area512-sprite'
require 'area512-sdfat'

class Scheduler
  W = 240 unless const_defined?(:W)
  H = 135 unless const_defined?(:H)
  SAVE_FILE = "/data/scheduler.txt" unless const_defined?(:SAVE_FILE)

  C_BACKGROUND = Widget.theme_background unless const_defined?(:C_BACKGROUND)
  C_BORDER = Widget.theme_border unless const_defined?(:C_BORDER)
  C_TEXT = Widget.theme_text unless const_defined?(:C_TEXT)
  C_SELECTED = Widget.theme_selected unless const_defined?(:C_SELECTED)
  C_BOX = Widget.theme_box unless const_defined?(:C_BOX)
  C_EMPHASIS = Widget.theme_emphasis unless const_defined?(:C_EMPHASIS)

  MONTHS = ["JAN", "FEB", "MAR", "APR", "MAY", "JUN",
            "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"] unless const_defined?(:MONTHS)
  WDAYS = ["SU", "MO", "TU", "WE", "TH", "FR", "SA"] unless const_defined?(:WDAYS)
end
