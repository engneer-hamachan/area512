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

  COL_BG = 0x000000 unless const_defined?(:COL_BG)
  COL_AMBER = 0xF5972D unless const_defined?(:COL_AMBER)
  COL_DIM = 0xCFA45F unless const_defined?(:COL_DIM)
  COL_GOLD = 0xFFD966 unless const_defined?(:COL_GOLD)

  C_BG = COL_BG unless const_defined?(:C_BG)
  C_GRID = COL_AMBER unless const_defined?(:C_GRID)
  C_HEAD = COL_BG unless const_defined?(:C_HEAD)
  C_SEL = COL_GOLD unless const_defined?(:C_SEL)
  C_TEXT = COL_DIM unless const_defined?(:C_TEXT)
  C_DIM = COL_DIM unless const_defined?(:C_DIM)
  C_EDIT = COL_BG unless const_defined?(:C_EDIT)
  C_ERR = COL_AMBER unless const_defined?(:C_ERR)

  HELP = "hjkl move e edit c clear g goto s save o open q quit" unless const_defined?(:HELP)
  SHEET_FILE = "sheet.txt" unless const_defined?(:SHEET_FILE)
end
