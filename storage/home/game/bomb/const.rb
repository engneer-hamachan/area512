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

  COL_BG = 0x000000 unless const_defined?(:COL_BG)
  COL_AMBER = 0xF5972D unless const_defined?(:COL_AMBER)
  COL_DIM = 0xCFA45F unless const_defined?(:COL_DIM)
  COL_GOLD = 0xFFD966 unless const_defined?(:COL_GOLD)

  C_BG = COL_BG unless const_defined?(:C_BG)
  C_PANEL = COL_BG unless const_defined?(:C_PANEL)
  C_GRID = COL_AMBER unless const_defined?(:C_GRID)
  C_CLOSED = COL_DIM unless const_defined?(:C_CLOSED)
  C_OPEN = COL_BG unless const_defined?(:C_OPEN)
  C_CURSOR = COL_GOLD unless const_defined?(:C_CURSOR)
  C_TEXT = COL_GOLD unless const_defined?(:C_TEXT)
  C_DIM = COL_DIM unless const_defined?(:C_DIM)
  C_FLAG = COL_GOLD unless const_defined?(:C_FLAG)
  C_MINE = COL_AMBER unless const_defined?(:C_MINE)
  C_WIN = COL_GOLD unless const_defined?(:C_WIN)
  C_LOSE = COL_AMBER unless const_defined?(:C_LOSE)
end

def rnd(n)
  return 0 if n <= 0
  RNG.random_int % n
end
