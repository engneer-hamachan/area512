class WidgetGallery
  def draw
    @sp.fill(Widget.bg)
    Widget.header(@sp, "WIDGET GALLERY", TABS[@tab])
    Widget.tabs(@sp, 18, TABS, @tab)

    case @tab
    when 0 then draw_parts
    when 1 then draw_table
    when 2 then draw_chart
    when 3 then draw_list
    when 4 then draw_text
    when 5 then draw_modal
    end

    Widget.page_dots(@sp, 113, TABS.length, @tab)
    Widget.footer(@sp, @message)
    @sp.push(0, 0)
    @busy_frame += 1
  end

  def draw_parts
    Widget.checkbox(@sp, 5, 36, "CHECK", @checked, @focus == 0)
    Widget.radio(@sp, 92, 36, "RADIO", @radio, @focus == 1)
    Widget.toggle(@sp, 184, 36, @toggle, @focus == 2)
    Widget.spinner(@sp, 5, 54, 82, SPINNER_VALUES[@spinner], @focus == 3)
    Widget.gauge(@sp, 96, 55, 75, @spinner + 1, SPINNER_VALUES.length)
    Widget.slider(@sp, 178, 54, 55, @spinner, SPINNER_VALUES.length - 1, @focus == 3)
    Widget.button(@sp, 5, 74, 70, "ACTION", @focus == 4)
    Widget.badge(@sp, 84, 75, "LIVE")
    Widget.battery(@sp, 147, 77)
    Widget.busy(@sp, 215, 75, @busy_frame)
    Widget.text_center(@sp, 94, "j/k focus  SPACE change  h/l value")
  end

  def draw_table
    widths = [40, 116, 76]

    Widget.table_header(@sp, 4, 36, widths, ["ID", "NAME", "QTY"])
    Widget.table_row(@sp, 4, 52, widths, ["1", "Bolt M3", "12"], false)
    Widget.table_row(@sp, 4, 68, widths, ["2", "Amber LED", "8"], true)
    Widget.field(@sp, 4, 88, 232, "MODE", "AUTOMATIC", false)
    Widget.field(@sp, 4, 103, 232, "OUTPUT", "ENABLED", true)
  end

  def draw_chart
    values = chart_values

    Widget.bar_chart(@sp, 8, 38, 100, 46, values, 10)
    Widget.line_chart(@sp, 130, 38, 100, 46, values, 10)
    Widget.text_center(@sp, 90, "h/l change sample values")
  end

  def chart_values
    base = [2, 6, 4, 9, 5, 7]
    shift = @chart_shift
    shift = 0 unless shift.is_a?(Integer)
    values = []
    index = 0

    while index < base.length
      values.push((base[index] + shift) % 11)
      index += 1
    end

    values
  end

  def draw_list
    @list.draw(@sp)
    Widget.text_center(@sp, 94, "m mark  x clear  UP/DOWN move")
  end

  def draw_text
    Widget.marquee(
      @sp,
      4,
      36,
      Display.width - 8,
      "MARQUEE: Area512Widget reusable UI toolkit",
      @busy_frame % 180
    )
    Widget.wrap_text(
      @sp,
      4,
      50,
      Display.width - 8,
      15,
      "wrap_text draws into a fixed rectangle."
    )
    @text_view.draw(@sp)
  end

  def draw_modal
    Widget.titled_panel(@sp, 18, 40, Display.width - 36, 54, "MODAL DEMO")
    Widget.text_center(@sp, 60, "ENTER opens the modal menu", Widget.gold)
    Widget.text_center(@sp, 78, "input / number / confirm / dialog / menu / alert")
  end
end
