class Breadboard
  def list_labels(labeled_items)
    labels = []
    i = 0
    while i < labeled_items.length
      labels.push(labeled_items[i][0])
      i += 1
    end
    labels
  end

  def list_category_labels
    ["Wire / passive", "LED / display", "Switch", "Power / output", "Semiconductor"]
  end

  def list_category_entries(category_index)
    case category_index
    when 0
      [["Jumper wire", 0, 0], ["Resistor", 1, 0], ["Volume 10k", 13, 0],
       ["CdS", 14, 0], ["Capacitor 1000uF", 15, 0]]
    when 1
      [["LED red", 2, 0], ["LED yellow", 2, 1], ["LED green", 2, 2],
       ["LED blue", 2, 3], ["RGB LED", 3, 0], ["7-segment LED", 4, 0]]
    when 2
      [["Tact switch", 6, 0], ["Slide switch", 7, 0], ["Relay", 12, 0]]
    when 3
      [["Battery 5V", 8, 0], ["Buzzer", 9, 0], ["Motor", 10, 0]]
    else
      [["Diode", 5, 0], ["NPN transistor", 11, 0]]
    end
  end

  def start_placement
    category_labels = list_category_labels
    category_index = Widget.menu(@sprite, "ADD PART", category_labels, @menu_category_index)
    return if category_index.nil?
    entries = list_category_entries(category_index)
    initial_entry_index = category_index == @menu_category_index ? @menu_entry_index : 0
    entry_index = Widget.menu(@sprite, category_labels[category_index], list_labels(entries), initial_entry_index)
    return if entry_index.nil?
    @menu_category_index = category_index
    @menu_entry_index = entry_index
    @placing_entry = entries[entry_index]
    @placing_first_hole_index = nil
  end

  def cancel_placement
    @placing_entry = nil
    @placing_first_hole_index = nil
  end

  def read_placing_kind
    @placing_entry[1]
  end

  def handle_placement_key(key)
    case key
    when "ENTER", " "
      confirm_placement
    when "ESC", "q"
      cancel_placement
    end
  end

  def compute_placement_hole_indices
    kind = read_placing_kind
    return compute_footprint_hole_indices(kind, @cursor_row, @cursor_column) if has_footprint?(kind)
    return [find_cursor_hole_index] if @placing_first_hole_index.nil?
    [@placing_first_hole_index, find_cursor_hole_index]
  end

  def placement_allowed?
    hole_indices = compute_placement_hole_indices
    !hole_indices.nil? && holes_free?(hole_indices)
  end

  def confirm_placement
    unless placement_allowed?
      @message = "blocked"
      return
    end
    kind = read_placing_kind
    if !has_footprint?(kind) && @placing_first_hole_index.nil?
      @placing_first_hole_index = find_cursor_hole_index
      return
    end
    add_part(kind, compute_placement_hole_indices, @placing_entry[2])
    cancel_placement
  end

  def format_placement_title
    kind = read_placing_kind
    label = @placing_entry[0]
    return "#{label}: place" if has_footprint?(kind)
    terminal_index = @placing_first_hole_index.nil? ? 0 : 1
    "#{label}: #{find_terminal_name(kind, terminal_index)}"
  end
end
