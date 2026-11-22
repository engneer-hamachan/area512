require 'area512-quaternary-sprite'
require 'area512-touch'

class Solitair
  TOUCH_BUTTON_COLUMN_LEFT = 241 unless const_defined?(:TOUCH_BUTTON_COLUMN_LEFT)
  TOUCH_BUTTON_COLUMN_TOP = -1 unless const_defined?(:TOUCH_BUTTON_COLUMN_TOP)
  TOUCH_BUTTON_COLUMN_WIDTH = 56 unless const_defined?(:TOUCH_BUTTON_COLUMN_WIDTH)
  TOUCH_BUTTON_COLUMN_HEIGHT = 137 unless const_defined?(:TOUCH_BUTTON_COLUMN_HEIGHT)
  TOUCH_BUTTON_COLUMN_GAP = 2 unless const_defined?(:TOUCH_BUTTON_COLUMN_GAP)
  TOUCH_BUTTON_FONT_SIZE = 8 unless const_defined?(:TOUCH_BUTTON_FONT_SIZE)
  TOUCH_BUTTON_CHARACTER_WIDTH = 6 unless const_defined?(:TOUCH_BUTTON_CHARACTER_WIDTH)
  TOUCH_BUTTON_LETTER_HEIGHT = 7 unless const_defined?(:TOUCH_BUTTON_LETTER_HEIGHT)
  TOUCH_BUTTON_PRESSED_MS = 80 unless const_defined?(:TOUCH_BUTTON_PRESSED_MS)

  def show_touch_button_column(labels)
    @touch_button_labels = labels
    @is_touch_button_on = []
    labels.length.times { @is_touch_button_on.push(false) }
    push_touch_button_column(-1)
  end

  def set_touch_button_on(button_index, is_on)
    return unless @touch_button_labels
    return if button_index < 0 || button_index >= @touch_button_labels.length

    @is_touch_button_on[button_index] = is_on
    push_touch_button_column(-1)
  end

  def find_touch_button_top(button_index)
    TOUCH_BUTTON_COLUMN_HEIGHT * button_index / @touch_button_labels.length
  end

  def push_touch_button_column(pressed_button_index)
    return unless @touch_button_labels

    sprite = QuaternarySprite.new(TOUCH_BUTTON_COLUMN_WIDTH, TOUCH_BUTTON_COLUMN_HEIGHT, TOUCH_BUTTON_FONT_SIZE)
    button_count = @touch_button_labels.length
    column_border_width = TOUCH_BUTTON_COLUMN_WIDTH - TOUCH_BUTTON_COLUMN_GAP
    sprite.fill(QuaternarySprite::BACKGROUND_COLOR_INDEX)
    button_index = 0

    while button_index < button_count
      button_top = find_touch_button_top(button_index)
      button_bottom = find_touch_button_top(button_index + 1)
      label_color_index = QuaternarySprite::TEXT_COLOR_INDEX
      fill_color_index = QuaternarySprite::BACKGROUND_COLOR_INDEX

      if button_index == pressed_button_index
        fill_color_index = QuaternarySprite::SELECTED_COLOR_INDEX
        label_color_index = QuaternarySprite::BACKGROUND_COLOR_INDEX
      elsif @is_touch_button_on[button_index]
        fill_color_index = QuaternarySprite::BOX_COLOR_INDEX
        label_color_index = QuaternarySprite::EMPHASIS_COLOR_INDEX
      end

      sprite.fill_rect(TOUCH_BUTTON_COLUMN_GAP, button_top, column_border_width, button_bottom - button_top, fill_color_index)

      if button_index > 0
        sprite.line(TOUCH_BUTTON_COLUMN_GAP, button_top, TOUCH_BUTTON_COLUMN_WIDTH - 1, button_top,
                    QuaternarySprite::BORDER_COLOR_INDEX)
      end

      label = @touch_button_labels[button_index]
      label_letter_width = label.length * TOUCH_BUTTON_CHARACTER_WIDTH - 1
      inside_left = TOUCH_BUTTON_COLUMN_GAP + 1
      inside_width = column_border_width - 2
      inside_top = button_top + 1
      inside_bottom = button_index == button_count - 1 ? button_bottom - 1 : button_bottom
      inside_height = inside_bottom - inside_top
      sprite.text(inside_left + (inside_width - label_letter_width) / 2,
                  inside_top + (inside_height - TOUCH_BUTTON_LETTER_HEIGHT) / 2,
                  label, label_color_index)
      button_index += 1
    end

    sprite.rect(TOUCH_BUTTON_COLUMN_GAP, 0, column_border_width, TOUCH_BUTTON_COLUMN_HEIGHT,
                QuaternarySprite::BORDER_COLOR_INDEX)
    Display.subwindow(TOUCH_BUTTON_COLUMN_LEFT, TOUCH_BUTTON_COLUMN_TOP,
                      TOUCH_BUTTON_COLUMN_WIDTH, TOUCH_BUTTON_COLUMN_HEIGHT)
    Display.push_subwindow(sprite)
    sprite.delete
  end

  def find_touch_button_index_at(window_x, window_y)
    return nil unless @touch_button_labels
    return nil if window_x < TOUCH_BUTTON_COLUMN_LEFT || window_x >= TOUCH_BUTTON_COLUMN_LEFT + TOUCH_BUTTON_COLUMN_WIDTH
    return nil if window_y < TOUCH_BUTTON_COLUMN_TOP || window_y >= TOUCH_BUTTON_COLUMN_TOP + TOUCH_BUTTON_COLUMN_HEIGHT

    button_index = 0
    column_y = window_y - TOUCH_BUTTON_COLUMN_TOP

    while button_index < @touch_button_labels.length - 1 && column_y >= find_touch_button_top(button_index + 1)
      button_index += 1
    end

    button_index
  end

  def read_touch_event
    while true
      event = Touch.read
      return nil unless event

      kind = event[0]

      if kind == "DRAG"
        if @is_touch_button_column_dragged.nil?
          @is_touch_button_column_dragged = !find_touch_button_index_at(event[1], event[2]).nil?
        end
        return event unless @is_touch_button_column_dragged
      elsif kind == "RELEASE"
        is_touch_button_column_dragged = @is_touch_button_column_dragged
        @is_touch_button_column_dragged = nil
        return event unless is_touch_button_column_dragged
      else
        button_index = find_touch_button_index_at(event[1], event[2])
        return event unless button_index

        push_touch_button_column(button_index)
        sleep_ms(TOUCH_BUTTON_PRESSED_MS)
        push_touch_button_column(-1)
        return ["BUTTON", button_index, 0]
      end
    end
  end
end
