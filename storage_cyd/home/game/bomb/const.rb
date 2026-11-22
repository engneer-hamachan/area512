require 'area512-widget'
require 'rng'
require 'area512-sprite'

# Constants live under the app class so they never collide with other apps in
# the shared sandbox VM (each run keeps a global const table). Guard with the
# class-scoped const_defined? to stay quiet on re-runs of this same app.
class Bomb
  MW = 9 unless const_defined?(:MW)
  MH = 9 unless const_defined?(:MH)
  MINES = 10 unless const_defined?(:MINES)

  CW = 14 unless const_defined?(:CW)
  CH = 12 unless const_defined?(:CH)
  OX = 57 unless const_defined?(:OX)
  OY = 16 unless const_defined?(:OY)

  COL_BACKGROUND = Widget.theme_background unless const_defined?(:COL_BACKGROUND)
  COL_BORDER = Widget.theme_border unless const_defined?(:COL_BORDER)
  COL_TEXT = Widget.theme_text unless const_defined?(:COL_TEXT)
  COL_SELECTED = Widget.theme_selected unless const_defined?(:COL_SELECTED)
  COL_EMPHASIS = Widget.theme_emphasis unless const_defined?(:COL_EMPHASIS)

  C_BG = COL_BACKGROUND unless const_defined?(:C_BG)
  C_PANEL = COL_BACKGROUND unless const_defined?(:C_PANEL)
  C_GRID = COL_BORDER unless const_defined?(:C_GRID)
  C_CLOSED = COL_TEXT unless const_defined?(:C_CLOSED)
  C_OPEN = COL_BACKGROUND unless const_defined?(:C_OPEN)
  C_CURSOR = COL_SELECTED unless const_defined?(:C_CURSOR)
  C_TEXT = COL_SELECTED unless const_defined?(:C_TEXT)
  C_DIM = COL_TEXT unless const_defined?(:C_DIM)
  C_FLAG = COL_SELECTED unless const_defined?(:C_FLAG)
  C_MINE = COL_EMPHASIS unless const_defined?(:C_MINE)
  C_WIN = COL_SELECTED unless const_defined?(:C_WIN)
  C_LOSE = COL_EMPHASIS unless const_defined?(:C_LOSE)
end

def rnd(n)
  return 0 if n <= 0
  RNG.random_int % n
end
