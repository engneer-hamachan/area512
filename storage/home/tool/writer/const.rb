# Tiny GUI-style word processor for small Area512 screens.

# Constants live under the app class so they never collide with other apps in
# the shared sandbox VM (each run keeps a global const table). Guard with the
# class-scoped const_defined? to stay quiet on re-runs of this same app.
class Writer
  DATA_DIR = "Area512_data" unless const_defined?(:DATA_DIR)
  DOC_FILE = "#{DATA_DIR}/writer.txt" unless const_defined?(:DOC_FILE)
  DOC_NAME = "writer.txt" unless const_defined?(:DOC_NAME)

  MARGIN = 6 unless const_defined?(:MARGIN)
  TOOL_H = 22 unless const_defined?(:TOOL_H)
  STATUS_H = 18 unless const_defined?(:STATUS_H)
  LINE_H = 14 unless const_defined?(:LINE_H)
  PAGE_PAD = 12 unless const_defined?(:PAGE_PAD)
  GLYPH_W = 6 unless const_defined?(:GLYPH_W)
  WIDE_W = 12 unless const_defined?(:WIDE_W)

  C_APP = 0xcfd6df unless const_defined?(:C_APP)
  C_TOOL = 0xf7f8fa unless const_defined?(:C_TOOL)
  C_TOOL_DARK = 0xaeb8c3 unless const_defined?(:C_TOOL_DARK)
  C_PAGE = 0xfffffb unless const_defined?(:C_PAGE)
  C_PAGE_EDGE = 0x9aa3ad unless const_defined?(:C_PAGE_EDGE)
  C_TEXT = 0x202428 unless const_defined?(:C_TEXT)
  C_DIM = 0x75808a unless const_defined?(:C_DIM)
  C_RULE = 0xd8dde2 unless const_defined?(:C_RULE)
  C_MARGIN_LINE = 0xd26a6a unless const_defined?(:C_MARGIN_LINE)
  C_ACCENT = 0x276fb3 unless const_defined?(:C_ACCENT)
  C_ACCENT_2 = 0x3b8b5b unless const_defined?(:C_ACCENT_2)
  C_STATUS = 0x1e4f86 unless const_defined?(:C_STATUS)
  C_STATUS_TEXT = 0xf2f6fa unless const_defined?(:C_STATUS_TEXT)
  C_CURSOR = 0x0d0f12 unless const_defined?(:C_CURSOR)

  HELP = "^Space IME  ^K/^J scroll  ^S save  ^O open  ^Q quit" unless const_defined?(:HELP)
end
