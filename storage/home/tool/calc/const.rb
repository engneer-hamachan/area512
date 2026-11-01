require 'area512-widget'

# Tiny spreadsheet for Cardputer/T-Deck style screens.

# Constants live under the app class so they never collide with other apps in
# the shared sandbox VM (each run keeps a global const table). Guard with the
# class-scoped const_defined? to stay quiet on re-runs of this same app.
class Calc
  ROWS = 26 unless const_defined?(:ROWS)
  COLS = 26 unless const_defined?(:COLS)
  VIEW_COLS = 3 unless const_defined?(:VIEW_COLS)
  ROW_H = 14 unless const_defined?(:ROW_H)
  HEAD_W = 20 unless const_defined?(:HEAD_W)
  TOP_H = ROW_H unless const_defined?(:TOP_H)

  COL_BACKGROUND = Widget.theme_background unless const_defined?(:COL_BACKGROUND)
  COL_BORDER = Widget.theme_border unless const_defined?(:COL_BORDER)
  COL_TEXT = Widget.theme_text unless const_defined?(:COL_TEXT)
  COL_SELECTED = Widget.theme_selected unless const_defined?(:COL_SELECTED)
  COL_EMPHASIS = Widget.theme_emphasis unless const_defined?(:COL_EMPHASIS)

  C_BG = COL_BACKGROUND unless const_defined?(:C_BG)
  C_GRID = COL_BORDER unless const_defined?(:C_GRID)
  C_HEAD = COL_BACKGROUND unless const_defined?(:C_HEAD)
  C_SEL = COL_SELECTED unless const_defined?(:C_SEL)
  C_TEXT = COL_TEXT unless const_defined?(:C_TEXT)
  C_DIM = COL_TEXT unless const_defined?(:C_DIM)
  C_EDIT = COL_BACKGROUND unless const_defined?(:C_EDIT)
  C_ERR = COL_EMPHASIS unless const_defined?(:C_ERR)

  HELP = "hjkl move e edit c clear g goto s save o open q quit" unless const_defined?(:HELP)
  SHEET_DIRECTORY = "/data/calc" unless const_defined?(:SHEET_DIRECTORY)
  SHEET_EXTENSION = ".txt" unless const_defined?(:SHEET_EXTENSION)
  LEGACY_SHEET_FILE = "/data/sheet.txt" unless const_defined?(:LEGACY_SHEET_FILE)
  LEGACY_SHEET_NAME = "sheet.txt" unless const_defined?(:LEGACY_SHEET_NAME)
end
