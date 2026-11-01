# Tiny GUI-style word processor for small Area512 screens.

# Constants live under the app class so they never collide with other apps in
# the shared sandbox VM (each run keeps a global const table). Guard with the
# class-scoped const_defined? to stay quiet on re-runs of this same app.
class Writer
  DOC_DIRECTORY = "/data/writer" unless const_defined?(:DOC_DIRECTORY)
  DOC_EXTENSION = ".txt" unless const_defined?(:DOC_EXTENSION)
  LEGACY_DOC_FILE = "/data/writer.txt" unless const_defined?(:LEGACY_DOC_FILE)
  LEGACY_DOC_NAME = "writer.txt" unless const_defined?(:LEGACY_DOC_NAME)

  LINE_H = 14 unless const_defined?(:LINE_H)
  PAGE_PAD = 4 unless const_defined?(:PAGE_PAD)
  SCROLLBAR_W = 3 unless const_defined?(:SCROLLBAR_W)
  GLYPH_W = 6 unless const_defined?(:GLYPH_W)
  WIDE_W = 12 unless const_defined?(:WIDE_W)

  HELP = "^Space IME  ^K/^J scroll  ^S save  ^O open  ^Q quit" unless const_defined?(:HELP)
end
