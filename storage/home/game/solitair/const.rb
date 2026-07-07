# Solitair -- constants and small card helpers

require 'rng'
require 'area512-sprite'
require 'area512-sdfat'
require 'machine'

# Constants live under the app class so they never collide with other apps in
# the shared sandbox VM (each run keeps a global const table). Guard with the
# class-scoped const_defined? to stay quiet on re-runs of this same app.
class Solitair
  CW = 5 unless const_defined?(:CW)
  CH = 10 unless const_defined?(:CH)
  CARD_W = 25 unless const_defined?(:CARD_W)
  CARD_H = 25 unless const_defined?(:CARD_H)
  COL_W = 30 unless const_defined?(:COL_W)
  STACK_STEP = 10 unless const_defined?(:STACK_STEP)
  TOP_Y = 12 unless const_defined?(:TOP_Y)
  TABLEAU_Y = 53 unless const_defined?(:TABLEAU_Y)
  MSG_Y = 124 unless const_defined?(:MSG_Y)
  VISIBLE_ROWS = 6 unless const_defined?(:VISIBLE_ROWS)
  BOARD_W = 7 * COL_W unless const_defined?(:BOARD_W)
  BOARD_H = 135 unless const_defined?(:BOARD_H)
  HIGHSCORE_FILE = "/data/solitair.txt" unless const_defined?(:HIGHSCORE_FILE)
  SCORE_TIME_BASE = 360000 unless const_defined?(:SCORE_TIME_BASE)

  COL_BG     = 0x000000 unless const_defined?(:COL_BG)
  COL_AMBER  = 0xF5972D unless const_defined?(:COL_AMBER)
  COL_DIM    = 0xCFA45F unless const_defined?(:COL_DIM)
  COL_GOLD   = 0xFFD966 unless const_defined?(:COL_GOLD)

  C_BG     = COL_BG unless const_defined?(:C_BG)
  C_PANEL  = COL_BG unless const_defined?(:C_PANEL)
  C_TEXT   = COL_DIM unless const_defined?(:C_TEXT)
  C_DIM    = COL_DIM unless const_defined?(:C_DIM)
  C_RED    = COL_AMBER unless const_defined?(:C_RED)
  C_BLACK  = COL_GOLD unless const_defined?(:C_BLACK)
  C_BACK   = COL_AMBER unless const_defined?(:C_BACK)
  C_SEL    = COL_GOLD unless const_defined?(:C_SEL)
  C_MSG    = COL_DIM unless const_defined?(:C_MSG)
  C_CARD   = COL_BG unless const_defined?(:C_CARD)
  C_FACE   = COL_DIM unless const_defined?(:C_FACE)
  C_EDGE   = COL_DIM unless const_defined?(:C_EDGE)
  C_SHADOW = COL_BG unless const_defined?(:C_SHADOW)
  C_EMPTY  = COL_BG unless const_defined?(:C_EMPTY)
  C_SCROLL = COL_AMBER unless const_defined?(:C_SCROLL)
end

def rnd(n)
  return 0 if n <= 0
  RNG.random_int % n
end

def card_id(c)
  c < 0 ? -c - 1 : c
end

def card_suit(c)
  card_id(c) / 13
end

def card_rank(c)
  card_id(c) % 13
end

def red_card?(c)
  s = card_suit(c)
  s == 1 || s == 2
end

def face_up?(c)
  c >= 0
end

def card_name(c)
  r = card_rank(c)
  s = card_suit(c)
  rn = "A"
  rn = "#{r + 1}" if r > 0 && r < 9
  rn = "T" if r == 9
  rn = "J" if r == 10
  rn = "Q" if r == 11
  rn = "K" if r == 12
  sn = "S"
  sn = "H" if s == 1
  sn = "D" if s == 2
  sn = "C" if s == 3
  rn + sn
end
