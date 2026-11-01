require 'area512-widget'

class SlideShow
  PROJECT_DIRECTORY = "/data/slide" unless const_defined?(:PROJECT_DIRECTORY)

  ROW_HEIGHT = 16 unless const_defined?(:ROW_HEIGHT)
  BODY_TOP = 18 unless const_defined?(:BODY_TOP)
  BODY_BOTTOM = 112 unless const_defined?(:BODY_BOTTOM)
  VISIBLE_ROW_COUNT = (BODY_BOTTOM - BODY_TOP) / ROW_HEIGHT unless const_defined?(:VISIBLE_ROW_COUNT)

  TEXT_COLOR = Widget.theme_text unless const_defined?(:TEXT_COLOR)
  HEADING_1_COLOR = Widget.theme_emphasis unless const_defined?(:HEADING_1_COLOR)
  HEADING_2_COLOR = Widget.theme_emphasis unless const_defined?(:HEADING_2_COLOR)
  HEADING_3_COLOR = Widget.theme_text unless const_defined?(:HEADING_3_COLOR)
  CODE_COLOR = Widget.theme_emphasis unless const_defined?(:CODE_COLOR)
  CODE_BACKGROUND_COLOR = Widget.theme_box unless const_defined?(:CODE_BACKGROUND_COLOR)
end
