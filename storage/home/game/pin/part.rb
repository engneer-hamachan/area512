class BreadboardPart
  attr_accessor :kind, :terminal_hole_indices, :terminal_node_indices, :junctions, :junction_states,
                :junction_currents, :color_index, :resistance_index, :knob_position,
                :light_level, :slide_position, :pressed_frames, :energized,
                :transistor_mode, :capacitor_voltage, :burned, :current, :voltage,
                :motor_phase, :levels

  def initialize(kind, terminal_hole_indices, color_index, junctions)
    @kind = kind
    @terminal_hole_indices = terminal_hole_indices
    @terminal_node_indices = []
    @junctions = junctions
    @junction_states = Array.new(junctions.length, false)
    @junction_currents = Array.new(junctions.length, 0.0)
    @color_index = color_index
    @slide_position = 0
    @pressed_frames = 0
    @energized = false
    @capacitor_voltage = 0.0
    @burned = false
    @current = 0.0
    @voltage = 0.0
    @motor_phase = 0
    @levels = []
  end
end
