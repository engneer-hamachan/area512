class WidgetGallery
  def handle(key)
    if key == "LEFT"
      change_tab(-1)
    elsif key == "RIGHT"
      change_tab(1)
    elsif key == "q"
      @quit = true
    else
      handle_current_tab(key)
    end
  end

  def change_tab(delta)
    @tab = (@tab + delta) % TABS.length
    @focus = 0
    @message = "LEFT/RIGHT tabs  q quit"
  end

  def handle_current_tab(key)
    case @tab
    when 0 then handle_parts(key)
    when 2 then handle_chart(key)
    when 3 then handle_list(key)
    when 4 then handle_text(key)
    when 5 then handle_modal(key)
    end
  end

  def handle_parts(key)
    focus = @focus
    focus = 0 unless focus.is_a?(Integer)

    if key == "UP" || key == "k" || key == ";"
      @focus = [focus - 1, 0].max
    elsif key == "DOWN" || key == "j" || key == "."
      @focus = [focus + 1, 4].min
    elsif key == "h"
      adjust_spinner(-1) if @focus == 3
    elsif key == "l"
      adjust_spinner(1) if @focus == 3
    elsif key == " " || key == "ENTER"
      activate_part
    end
  end

  def adjust_spinner(delta)
    @spinner = [[@spinner + delta, 0].max, SPINNER_VALUES.length - 1].min
    @message = "Spinner: " + SPINNER_VALUES[@spinner]
  end

  def activate_part
    case @focus
    when 0
      @checked = !@checked
      @message = "Checkbox changed"
    when 1
      @radio = !@radio
      @message = "Radio changed"
    when 2
      @toggle = !@toggle
      @message = "Toggle changed"
    when 3
      adjust_spinner(1)
    when 4
      @message = "Button pressed"
    end
  end

  def handle_chart(key)
    if key == "h"
      @chart_shift -= 1
      @message = "Chart values shifted down"
    elsif key == "l"
      @chart_shift += 1
      @message = "Chart values shifted up"
    end
  end

  def handle_list(key)
    return if @list.handle(key)

    if key == "m"
      @list.toggle_mark
      @message = "Mark toggled"
    elsif key == "x"
      @list.clear
      @message = "List cleared"
    end
  end

  def handle_text(key)
    @message = "Text view scrolled" if @text_view.handle(key)
  end

  def handle_modal(key)
    return unless key == "ENTER" || key == " "

    choices = ["Input", "Number", "Confirm", "Dialog", "Menu", "Alert"]
    selected = Widget.menu(@sp, "MODALS", choices, 0)
    run_modal(selected) if selected
  end

  def run_modal(selected)
    case selected
    when 0
      result = Widget.input(@sp, "NAME:", "Area512")
      @message = result ? "Input: " + result : "Input cancelled"
    when 1
      result = Widget.input_number(@sp, "VALUE:", 5, 0, 10)
      @message = result ? "Number: " + result.to_s : "Number cancelled"
    when 2
      @message = Widget.confirm(@sp, "Continue?") ? "Confirmed" : "Not confirmed"
    when 3
      result = Widget.dialog(@sp, "SELECT", ["OK", "Cancel"])
      @message = result ? "Dialog index: " + result.to_s : "Dialog cancelled"
    when 4
      result = Widget.menu(@sp, "FILES", ["Open", "Save", "Close"], 1)
      @message = result ? "Menu index: " + result.to_s : "Menu cancelled"
    when 5
      Widget.alert(@sp, "Alert example")
      @message = "Alert closed"
    end
  end
end
