class WidgetGallery
  TABS = ["PARTS", "TABLE", "CHART", "LIST", "TEXT", "MODAL"] unless const_defined?(:TABS)
  SPINNER_VALUES = ["LOW", "MID", "HIGH"] unless const_defined?(:SPINNER_VALUES)

  def initialize
    @tab = 0
    @focus = 0
    @checked = true
    @radio = false
    @toggle = true
    @spinner = 1
    @chart_shift = 0
    @busy_frame = 0
    @message = "LEFT/RIGHT tabs  q quit"
    @quit = false

    initialize_list
    initialize_text_view
  end

  def initialize_list
    @list = WidgetList.new
    @list.area(4, 36, Display.width - 8, 72)
    @list.empty_text = "No gallery rows"
    @list.show_marks = true

    12.times do |index|
      number = (index + 1).to_s
      @list.add("Gallery item " + number, "T" + number)
    end
  end

  def initialize_text_view
    @text_view = WidgetTextView.new
    @text_view.area(4, 65, Display.width - 8, 42)
    @text_view.text = "WidgetTextView wraps words using measured glyph widths. Use UP and DOWN to scroll this text. Japanese text: 日本語も実測幅で折り返します。"
  end
end
