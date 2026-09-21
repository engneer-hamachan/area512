require 'area512-widget'

class ThemeEditor
  THEME_PATH = "/etc/theme" unless const_defined?(:THEME_PATH)
  IMAGE_DIRECTORY = "/share/backgrounds" unless const_defined?(:IMAGE_DIRECTORY)
  IMAGE_EXTENSION = ".rgb565" unless const_defined?(:IMAGE_EXTENSION)

  COLOR_KEYS = ["background", "text", "emphasis", "border", "selected", "box"] unless const_defined?(:COLOR_KEYS)
  DEFAULT_COLORS = [0x000000, 0xCFA45F, 0xF5972D, 0xF5972D, 0xFFD966, 0x241604] unless const_defined?(:DEFAULT_COLORS)
  BACKGROUND_INDEX = 0 unless const_defined?(:BACKGROUND_INDEX)
  TEXT_INDEX = 1 unless const_defined?(:TEXT_INDEX)
  EMPHASIS_INDEX = 2 unless const_defined?(:EMPHASIS_INDEX)
  BORDER_INDEX = 3 unless const_defined?(:BORDER_INDEX)
  SELECTED_INDEX = 4 unless const_defined?(:SELECTED_INDEX)
  BOX_INDEX = 5 unless const_defined?(:BOX_INDEX)
  IMAGE_ROW_INDEX = COLOR_KEYS.length unless const_defined?(:IMAGE_ROW_INDEX)
  ROW_COUNT = COLOR_KEYS.length + 1 unless const_defined?(:ROW_COUNT)

  CHANNEL_NAMES = ["R", "G", "B"] unless const_defined?(:CHANNEL_NAMES)
  CHANNEL_SHIFTS = [16, 8, 0] unless const_defined?(:CHANNEL_SHIFTS)
  HEX_DIGITS = "0123456789ABCDEF" unless const_defined?(:HEX_DIGITS)

  ROW_HEIGHT = 14 unless const_defined?(:ROW_HEIGHT)
  LABEL_X = 4 unless const_defined?(:LABEL_X)
  SWATCH_X = 72 unless const_defined?(:SWATCH_X)
  SWATCH_WIDTH = 30 unless const_defined?(:SWATCH_WIDTH)
  HEX_X = 110 unless const_defined?(:HEX_X)
end
