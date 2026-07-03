require 'area512-sprite'

# Constants live under the app class so they never collide with other apps in
# the shared sandbox VM (each run keeps a global const table). Guard with the
# class-scoped const_defined? to stay quiet on re-runs of this same app.
class PaintApp
  APP_W = 240 unless const_defined?(:APP_W)
  APP_H = 135 unless const_defined?(:APP_H)

  STATUS_H = 15 unless const_defined?(:STATUS_H)
  CELL = 2 unless const_defined?(:CELL)
  CANVAS_W = APP_W / CELL unless const_defined?(:CANVAS_W)
  CANVAS_H = (APP_H - STATUS_H) / CELL unless const_defined?(:CANVAS_H)
  CANVAS_Y = STATUS_H unless const_defined?(:CANVAS_Y)

  BG_COLOR = 0x111111 unless const_defined?(:BG_COLOR)
  GRID_COLOR = 0x222222 unless const_defined?(:GRID_COLOR)
  TEXT_COLOR = 0xDDDDDD unless const_defined?(:TEXT_COLOR)
  CURSOR_COLOR = 0xFFFFFF unless const_defined?(:CURSOR_COLOR)
  ERASER_COLOR = 0x000000 unless const_defined?(:ERASER_COLOR)
  ERASER_INDEX = 7 unless const_defined?(:ERASER_INDEX)

  unless const_defined?(:PALETTE)
    PALETTE = [
      0xFFFFFF,
      0xFF5544,
      0xFFCC33,
      0x39FF00,
      0x33CCFF,
      0xC0C0FF,
      0xFF66FF,
      0x000000
    ]
  end

  unless const_defined?(:PALETTE_MARK)
    PALETTE_MARK = [
      "0",
      "1",
      "2",
      "3",
      "4",
      "5",
      "6",
      "7"
    ]
  end

  unless const_defined?(:PALETTE_NAME)
    PALETTE_NAME = [
      "white",
      "red",
      "gold",
      "green",
      "cyan",
      "blue",
      "pink",
      "black"
    ]
  end
end

def clamp(value, min, max)
  return min if value < min
  return max if value > max
  value
end

def absi(value)
  value < 0 ? -value : value
end
