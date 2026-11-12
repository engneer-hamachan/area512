#include "area512_quaternary_crt.h"

#include <string.h>

static constexpr int LINE_CAPACITY = 320;
static constexpr int RED_BLOOM_REACH = 4;
static constexpr int BLOOM_THRESHOLD = 128;
static constexpr int PALETTE_COLOR_COUNT = 16;
// Source rows from two above to one below the output row.
static constexpr int SOURCE_ROW_COUNT = 4;
// The dither matrix repeats every four rows.
static constexpr int DITHER_ROW_PERIOD = 4;
// Column entries start one column left of the first output column.
static constexpr int COLUMN_ENTRY_CAPACITY = LINE_CAPACITY + 1 + RED_BLOOM_REACH;
static constexpr int REPEAT_MASK_WORD_CAPACITY = (COLUMN_ENTRY_CAPACITY + 31) / 32;

static constexpr int
clamp(int value, int low, int high) {
  return value < low ? low : (value > high ? high : value);
}

// A column color packs 8-bit red, green and blue from the lowest byte up,
// sets COLOR_TRANSPARENT_BIT when the pixel is the transparent index, and
// counts in the bits from COLOR_RUN_SHIFT how many directly preceding entries
// are identical. The count may wrap or start low; it only has to never
// exceed the real run, because a long enough run lets a pixel reuse the
// previous pixel's result.
static constexpr uint32_t COLOR_TRANSPARENT_BIT = 1u << 24;
static constexpr int COLOR_RUN_SHIFT = 25;
static constexpr uint32_t COLOR_RUN_INCREMENT = 1u << COLOR_RUN_SHIFT;
// An output pixel reads the entries from one column left to RED_BLOOM_REACH
// columns right, so it repeats the previous pixel's unclamped value when that
// many entries plus one more on the left are identical, and repeats the
// output STRIPE_DITHER_PERIOD columns back, which has the same stripe and
// dither, when that many more are identical.
static constexpr int STRIPE_DITHER_PERIOD = 12;
static constexpr uint32_t REPEATED_INPUT_RUN = 1 + RED_BLOOM_REACH + 1;
static constexpr uint32_t REPEATED_OUTPUT_RUN =
  REPEATED_INPUT_RUN + STRIPE_DITHER_PERIOD - 1;

namespace {
struct ColumnEntry {
  // Weighted red bloom of the column in the upper half, weighted green bloom
  // in the lower half, so horizontal taps of both colors are summed together.
  uint32_t bloom_sum;
  uint32_t color;
};

struct TintTables {
  uint16_t tint_by_neutral[256][4];
};

struct StripeTables {
  uint8_t unlit[3][256];
};

// Built from the sprite palette at each push. Each palette color is reduced
// to RGB565 first, so the output equals area512_crt_push of an RGB565 sprite.
struct PaletteTables {
  uint32_t colors[PALETTE_COLOR_COUNT];
  uint8_t red_blooms[PALETTE_COLOR_COUNT];
  uint8_t green_blooms[PALETTE_COLOR_COUNT];
  // Bit n is set when palette index n has red or green bloom.
  uint32_t blooming_index_mask;
};
}

// crt.jpg spreads red to the left and below a bright stroke, green one
// pixel to the right, and keeps blue sharp, giving yellow-orange-red edges.
// Vertical weights are for dy = -2..2; horizontal for dx = -1..4 (red) and
// dx = -1..1 (green), where dx and dy are source offsets from the output.
static constexpr int red_vertical_weight[] = {2, 3, 3, 1, 0};
static constexpr int red_horizontal_weight[] = {1, 2, 3, 3, 2, 1};
static constexpr int green_vertical_weight[] = {1, 2, 1};
static constexpr int green_horizontal_weight[] = {2, 1, 1};
static constexpr int red_bloom_gain = 5;
static constexpr int green_bloom_gain = 3;
static constexpr int white_gain[] = {152, 185, 180};
static constexpr int black_level[] = {14, 15, 19};
static constexpr int unlit_stripe_gain[] = {218, 190, 140};
static constexpr int dither[] =
  {0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5};

static constexpr TintTables
build_tint_tables() {
  TintTables tables{};
  for (int neutral = 0; neutral < 256; ++neutral)
    for (int component = 0; component < 3; ++component)
      tables.tint_by_neutral[neutral][component] =
        (uint16_t)(256 - (256 - white_gain[component]) * neutral / 255);
  return tables;
}

static constexpr TintTables tint_tables = build_tint_tables();

static constexpr StripeTables
build_stripe_tables() {
  StripeTables tables{};
  for (int component = 0; component < 3; ++component)
    for (int value = 0; value < 256; ++value)
      tables.unlit[component][value] =
        (uint8_t)(value * unlit_stripe_gain[component] / 256);
  return tables;
}

static constexpr StripeTables stripe_tables = build_stripe_tables();

static_assert(
  (green_vertical_weight[0] + green_vertical_weight[1] +
   green_vertical_weight[2]) * 127 *
      (red_horizontal_weight[0] + red_horizontal_weight[1] +
       red_horizontal_weight[2] + red_horizontal_weight[3] +
       red_horizontal_weight[4] + red_horizontal_weight[5]) <
    65536,
  "green bloom must not carry into red when summing red taps"
);
static_assert(
  255 * unlit_stripe_gain[0] / 256 + 15 * 8 / 16 <= 255 &&
    255 * unlit_stripe_gain[1] / 256 + 15 * 4 / 16 <= 255 &&
    255 * unlit_stripe_gain[2] / 256 + 15 * 8 / 16 <= 255,
  "unlit stripes cannot overflow after dithering"
);

static PaletteTables
build_palette_tables(const lgfx::bgr888_t *palette) {
  PaletteTables tables{};
  for (int index = 0; index < PALETTE_COLOR_COUNT; ++index) {
    const int red = (palette[index].R8() >> 3) * 255 / 31;
    const int green = (palette[index].G8() >> 2) * 255 / 63;
    const int blue = (palette[index].B8() >> 3) * 255 / 31;
    tables.colors[index] =
      (uint32_t)red | (uint32_t)green << 8 | (uint32_t)blue << 16;
    tables.red_blooms[index] = (uint8_t)clamp(red - BLOOM_THRESHOLD, 0, 255);
    tables.green_blooms[index] =
      (uint8_t)clamp(green - BLOOM_THRESHOLD, 0, 255);
    if (tables.red_blooms[index] != 0 || tables.green_blooms[index] != 0)
      tables.blooming_index_mask |= 1u << index;
  }
  return tables;
}

// Two pixels share a byte, the left one in the high nibble.
__attribute__((always_inline)) static inline uint32_t
load_palette_index(const uint8_t *row, int column) {
  return row[column >> 1] >> ((~column & 1) << 2) & 0x0f;
}

// Row pointers address the first column of each row.
template <bool Transparent>
__attribute__((noinline)) static void
measure_column(
  const PaletteTables &palette_tables,
  const uint8_t *const rows[SOURCE_ROW_COUNT],
  int column,
  uint32_t transparent_index,
  ColumnEntry *entry
) {
  const uint32_t above2 = load_palette_index(rows[0], column);
  const uint32_t above = load_palette_index(rows[1], column);
  const uint32_t center = load_palette_index(rows[2], column);
  const uint32_t below = load_palette_index(rows[3], column);
  uint32_t bloom_sum = 0;
  if (((1u << above2) | (1u << above) | (1u << center) | (1u << below)) &
      palette_tables.blooming_index_mask) {
    uint32_t above2_red = palette_tables.red_blooms[above2];
    uint32_t above_red = palette_tables.red_blooms[above];
    uint32_t above_green = palette_tables.green_blooms[above];
    uint32_t center_red = palette_tables.red_blooms[center];
    uint32_t center_green = palette_tables.green_blooms[center];
    uint32_t below_red = palette_tables.red_blooms[below];
    uint32_t below_green = palette_tables.green_blooms[below];
    if (Transparent) {
      if (above2 == transparent_index)
        above2_red = 0;
      if (above == transparent_index)
        above_red = above_green = 0;
      if (center == transparent_index)
        center_red = center_green = 0;
      if (below == transparent_index)
        below_red = below_green = 0;
    }
    const uint32_t red = red_vertical_weight[0] * above2_red +
                         red_vertical_weight[1] * above_red +
                         red_vertical_weight[2] * center_red +
                         red_vertical_weight[3] * below_red;
    const uint32_t green = green_vertical_weight[0] * above_green +
                           green_vertical_weight[1] * center_green +
                           green_vertical_weight[2] * below_green;
    bloom_sum = red << 16 | green;
  }
  uint32_t color = palette_tables.colors[center];
  if (Transparent && center == transparent_index)
    color |= COLOR_TRANSPARENT_BIT;
  entry->bloom_sum = bloom_sum;
  entry->color = color;
}

// Bit n of a repeat mask is set when column first_column + n of a source row
// equals the column before it. Bit 0 stays clear so that the first column is
// always measured, and bits past the last column stay clear.
static void
build_repeat_mask(
  const uint8_t *row,
  int first_column,
  int column_count,
  uint32_t *mask
) {
  int column = first_column;
  uint32_t previous = load_palette_index(row, column);
  for (int remaining = column_count; remaining > 0; remaining -= 32) {
    const int word_column_count = remaining < 32 ? remaining : 32;
    uint32_t word = 0;
    for (int word_column = 0; word_column < word_column_count; ++word_column) {
      const uint32_t current = load_palette_index(row, column);
      word = word >> 1 | (current == previous ? 0x80000000u : 0);
      previous = current;
      ++column;
    }
    *mask++ = word >> (32 - word_column_count);
  }
  mask[-(column_count + 31) / 32] &= ~1u;
}

// A column whose four pixels equal the previous column's repeats its entry,
// so runs of such columns are found from the repeat masks and copied.
template <bool Transparent>
__attribute__((noinline)) static void
measure_columns(
  const PaletteTables &palette_tables,
  const uint8_t *const rows[SOURCE_ROW_COUNT],
  const uint32_t *const repeat_masks[SOURCE_ROW_COUNT],
  int first_column,
  int column_count,
  uint32_t transparent_index,
  ColumnEntry *entries
) {
  ColumnEntry *entry = entries;
  for (int first = 0; first < column_count; first += 32) {
    const int word_index = first / 32;
    uint32_t repeats = repeat_masks[0][word_index] &
                       repeat_masks[1][word_index] &
                       repeat_masks[2][word_index] &
                       repeat_masks[3][word_index];
    const ColumnEntry *const word_end =
      entries + (first + 32 < column_count ? first + 32 : column_count);
    while (entry < word_end) {
      if (!(repeats & 1)) {
        measure_column<Transparent>(
          palette_tables, rows, first_column + (int)(entry - entries),
          transparent_index, entry
        );
        ++entry;
        repeats >>= 1;
        continue;
      }
      const int run_length = ~repeats == 0 ? 32 : __builtin_ctz(~repeats);
      repeats = run_length == 32 ? 0 : repeats >> run_length;
      const uint32_t bloom_sum = entry[-1].bloom_sum;
      uint32_t color = entry[-1].color;
      for (int column = 0; column < run_length; ++column) {
        color += COLOR_RUN_INCREMENT;
        entry[column].bloom_sum = bloom_sum;
        entry[column].color = color;
      }
      entry += run_length;
    }
  }
}

template <int Component, int LitComponent>
__attribute__((always_inline)) static inline uint32_t
apply_stripe_and_dither(uint32_t value, uint32_t dither_value) {
  if (Component != LitComponent)
    return stripe_tables.unlit[Component][value < 255 ? value : 255] +
           dither_value;
  value += dither_value;
  return value < 255 ? value : 255;
}

namespace {
struct UnclampedPixel {
  uint32_t red;
  uint32_t green;
  uint32_t blue;
};
}

// entries starts one column left of the output column. unclamped keeps the
// previous pixel's value, which is reused when all inputs repeat.
template <int LitComponent, bool Transparent, bool CoreShiftsGreenAndNarrowsBlue>
__attribute__((always_inline)) static inline void
filter_pixel(
  const ColumnEntry *entries,
  const uint8_t *red_blue_dither,
  UnclampedPixel *unclamped,
  uint16_t *output
) {
  const uint32_t run = entries[1 + RED_BLOOM_REACH].color >> COLOR_RUN_SHIFT;
  if (run >= REPEATED_OUTPUT_RUN) {
    *output = output[-STRIPE_DITHER_PERIOD];
    return;
  }
  if (run < REPEATED_INPUT_RUN) {
    const uint32_t red_sum =
      (red_horizontal_weight[0] * entries[0].bloom_sum +
       red_horizontal_weight[1] * entries[1].bloom_sum +
       red_horizontal_weight[2] * entries[2].bloom_sum +
       red_horizontal_weight[3] * entries[3].bloom_sum +
       red_horizontal_weight[4] * entries[4].bloom_sum +
       red_horizontal_weight[5] * entries[5].bloom_sum) >>
      16;
    const uint32_t green_sum =
      (green_horizontal_weight[0] * entries[0].bloom_sum +
       green_horizontal_weight[1] * entries[1].bloom_sum +
       green_horizontal_weight[2] * entries[2].bloom_sum) &
      0xffff;
    const uint32_t red_halo = red_sum * red_bloom_gain / (9 * 12);
    const uint32_t green_halo = green_sum * green_bloom_gain / (4 * 4);
    const uint32_t center = entries[1].color;
    const uint32_t left = Transparent && (entries[0].color & COLOR_TRANSPARENT_BIT)
                            ? center
                            : entries[0].color;
    const uint32_t left_green = left >> 8 & 0xff;
    if ((center & 0xffffff) == 0) {
      unclamped->red = red_halo;
      unclamped->green =
        (CoreShiftsGreenAndNarrowsBlue ? left_green / 2 : 0) + green_halo;
      unclamped->blue = 0;
    } else {
      const uint32_t right =
        Transparent && (entries[2].color & COLOR_TRANSPARENT_BIT)
          ? center
          : entries[2].color;
      const uint32_t left_blue = left >> 16 & 0xff;
      const uint32_t right_blue = right >> 16 & 0xff;
      const uint32_t red = center & 0xff;
      const uint32_t green = center >> 8 & 0xff;
      const uint32_t blue = center >> 16 & 0xff;
      uint32_t neutral = red < green ? red : green;
      neutral = blue < neutral ? blue : neutral;
      // Green lags one pixel to the right and blue is narrower, so the
      // inner edge of a white stroke turns yellow before the red fringe.
      uint32_t narrow_blue = left_blue < blue ? left_blue : blue;
      narrow_blue = right_blue < narrow_blue ? right_blue : narrow_blue;
      const uint16_t *tint = tint_tables.tint_by_neutral[neutral];
      // Bloom lights the surroundings, not the already bright core.
      unclamped->red = red * tint[0] / 256 + red_halo * (255 - red) / 255 +
                       black_level[0];
      const uint32_t core_green =
        CoreShiftsGreenAndNarrowsBlue ? (green + left_green) / 2 : green;
      const uint32_t core_blue =
        CoreShiftsGreenAndNarrowsBlue ? (narrow_blue * 3 + blue) / 4 : blue;
      unclamped->green = core_green * tint[1] / 256 +
                         green_halo * (255 - green) / 255 + black_level[1];
      unclamped->blue = core_blue * tint[2] / 256 + black_level[2];
    }
  }
  // Fixed ordered dithering retains dark red/blue values in RGB565
  // without a green cast or time-dependent flicker on partial redraws.
  const uint32_t red =
    apply_stripe_and_dither<0, LitComponent>(unclamped->red, *red_blue_dither);
  const uint32_t green = apply_stripe_and_dither<1, LitComponent>(
    unclamped->green, *red_blue_dither / 2
  );
  const uint32_t blue = apply_stripe_and_dither<2, LitComponent>(
    unclamped->blue, *red_blue_dither
  );
  const uint32_t high_byte = (red >> 3) * 8 + (green >> 5);
  const uint32_t low_byte = (green >> 2 & 7) * 32 + (blue >> 3);
  *output = (uint16_t)(high_byte | low_byte << 8);
}

template <bool Transparent, bool CoreShiftsGreenAndNarrowsBlue>
static void
filter_pixel_at_phase(
  int phase,
  const ColumnEntry *entries,
  const uint8_t *red_blue_dither,
  UnclampedPixel *unclamped,
  uint16_t *output
) {
  if (phase == 0)
    filter_pixel<0, Transparent, CoreShiftsGreenAndNarrowsBlue>(
      entries, red_blue_dither, unclamped, output
    );
  else if (phase == 1)
    filter_pixel<1, Transparent, CoreShiftsGreenAndNarrowsBlue>(
      entries, red_blue_dither, unclamped, output
    );
  else
    filter_pixel<2, Transparent, CoreShiftsGreenAndNarrowsBlue>(
      entries, red_blue_dither, unclamped, output
    );
}

// red_blue_dithers holds the dither of every output column.
template <bool Transparent, bool CoreShiftsGreenAndNarrowsBlue>
__attribute__((noinline)) static void
filter_row(
  const ColumnEntry *entries,
  int column_count,
  int first_phase,
  const uint8_t *red_blue_dithers,
  uint16_t *output
) {
  UnclampedPixel unclamped = {0, 0, 0};
  int column = 0;
  int phase = first_phase;
  for (; column < column_count && phase != 0; ++column) {
    filter_pixel_at_phase<Transparent, CoreShiftsGreenAndNarrowsBlue>(
      phase, entries + column, &red_blue_dithers[column], &unclamped,
      output + column
    );
    phase = phase == 2 ? 0 : phase + 1;
  }
  for (; column + 3 <= column_count; column += 3) {
    filter_pixel<0, Transparent, CoreShiftsGreenAndNarrowsBlue>(
      entries + column, &red_blue_dithers[column], &unclamped,
      output + column
    );
    filter_pixel<1, Transparent, CoreShiftsGreenAndNarrowsBlue>(
      entries + column + 1, &red_blue_dithers[column + 1], &unclamped,
      output + column + 1
    );
    filter_pixel<2, Transparent, CoreShiftsGreenAndNarrowsBlue>(
      entries + column + 2, &red_blue_dithers[column + 2], &unclamped,
      output + column + 2
    );
  }
  for (phase = 0; column < column_count; ++column, ++phase) {
    filter_pixel_at_phase<Transparent, CoreShiftsGreenAndNarrowsBlue>(
      phase, entries + column, &red_blue_dithers[column], &unclamped,
      output + column
    );
  }
}

static const uint8_t *
find_source_row(const uint8_t *pixels, int stride, int height, int row) {
  return pixels + clamp(row, 0, height - 1) * stride;
}

static bool
does_source_row_repeat(
  const uint8_t *pixels,
  int stride,
  int height,
  int row,
  int byte_offset,
  int byte_count
) {
  const uint8_t *current = find_source_row(pixels, stride, height, row);
  const uint8_t *previous = find_source_row(pixels, stride, height, row - 1);
  return current == previous ||
         memcmp(current + byte_offset, previous + byte_offset, byte_count) == 0;
}

static void
push_output_row(
  lgfx::LGFX_Device *display,
  const uint16_t outputs[DITHER_ROW_PERIOD][LINE_CAPACITY],
  int screen_x,
  int screen_y,
  int column_count
) {
  display->pushImage(
    screen_x, screen_y, column_count, 1,
    reinterpret_cast<const lgfx::swap565_t *>(
      outputs[screen_y & (DITHER_ROW_PERIOD - 1)]
    )
  );
}

// The entries of a row come from source rows row - 2 to row + 1, so a row
// reuses the previous row's entries when those four source rows each repeat
// the source row above, and reuses the output of DITHER_ROW_PERIOD rows back,
// which has the same dither, when that many rows in succession reused their
// entries.
template <bool Transparent>
static void
push_rows(
  lgfx::LGFX_Device *display,
  const PaletteTables &palette_tables,
  const uint8_t *pixels,
  int width,
  int height,
  int x,
  int y,
  bool core_shifts_green_and_narrows_blue,
  uint32_t transparent_index,
  int first_row,
  int end_row
) {
  ColumnEntry entries[COLUMN_ENTRY_CAPACITY];
  // Panel byte order, as lgfx::swap565_t, without its zeroing constructor.
  uint16_t outputs[DITHER_ROW_PERIOD][LINE_CAPACITY];
  // Bytes in memory order give the red and blue dither of each column.
  uint32_t dither_words[LINE_CAPACITY / 4];
  // Source row index modulo SOURCE_ROW_COUNT selects the slot, which is
  // unique within the consecutive source rows of one output row.
  uint32_t repeat_masks[SOURCE_ROW_COUNT][REPEAT_MASK_WORD_CAPACITY];
  int repeat_mask_rows[SOURCE_ROW_COUNT];
  const int stride = (width + 1) / 2;
  for (int start = 0; start < width; start += LINE_CAPACITY) {
    const int end = clamp(start + LINE_CAPACITY, 0, width);
    const int entry_first = start - 1;
    const int entry_end = end + RED_BLOOM_REACH;
    const int measured_first = entry_first < 0 ? 0 : entry_first;
    const int measured_end = entry_end > width ? width : entry_end;
    // A byte shared with an unmeasured column only makes a row compare
    // unequal more often, which just measures it again.
    const int measured_byte_offset = measured_first / 2;
    const int measured_byte_count =
      (measured_end + 1) / 2 - measured_byte_offset;
    ColumnEntry *measured = entries + measured_first - entry_first;
    const int screen_x = x + start;
    for (int slot = 0; slot < SOURCE_ROW_COUNT; ++slot)
      repeat_mask_rows[slot] = -1;
    bool has_entries = false;
    int repeated_source_rows = 0;
    int repeated_entry_rows = 0;
    int unsent_row = first_row;
    for (int row = first_row; row < end_row; ++row) {
      const int screen_y = y + row;
      if (!Transparent && row - unsent_row == DITHER_ROW_PERIOD) {
        push_output_row(display, outputs, screen_x, y + unsent_row, end - start);
        ++unsent_row;
      }
      if (has_entries) {
        repeated_source_rows =
          does_source_row_repeat(
            pixels, stride, height, row + 1, measured_byte_offset,
            measured_byte_count
          )
            ? repeated_source_rows + 1
            : 0;
      } else {
        repeated_source_rows = 0;
        while (
          repeated_source_rows < SOURCE_ROW_COUNT &&
          does_source_row_repeat(
            pixels, stride, height, row + 1 - repeated_source_rows,
            measured_byte_offset, measured_byte_count
          )
        )
          ++repeated_source_rows;
      }
      if (has_entries && repeated_source_rows >= SOURCE_ROW_COUNT) {
        ++repeated_entry_rows;
      } else {
        const uint8_t *source_rows[SOURCE_ROW_COUNT];
        const uint32_t *source_repeat_masks[SOURCE_ROW_COUNT];
        for (int index = 0; index < SOURCE_ROW_COUNT; ++index) {
          const int source_row = clamp(row - 2 + index, 0, height - 1);
          const int slot = source_row & (SOURCE_ROW_COUNT - 1);
          source_rows[index] = pixels + source_row * stride;
          if (repeat_mask_rows[slot] != source_row) {
            build_repeat_mask(
              source_rows[index], measured_first,
              measured_end - measured_first, repeat_masks[slot]
            );
            repeat_mask_rows[slot] = source_row;
          }
          source_repeat_masks[index] = repeat_masks[slot];
        }
        measure_columns<Transparent>(
          palette_tables, source_rows, source_repeat_masks, measured_first,
          measured_end - measured_first, transparent_index, measured
        );
        // Clamped columns copy the edge entry with a zero run, which never
        // exceeds the real run.
        for (ColumnEntry *entry = entries; entry < measured; ++entry) {
          entry->bloom_sum = measured->bloom_sum;
          entry->color = measured->color & (COLOR_RUN_INCREMENT - 1);
        }
        const ColumnEntry *last = &entries[measured_end - 1 - entry_first];
        for (ColumnEntry *entry = &entries[measured_end - entry_first];
             entry < entries + (entry_end - entry_first);
             ++entry) {
          entry->bloom_sum = last->bloom_sum;
          entry->color = last->color & (COLOR_RUN_INCREMENT - 1);
        }
        has_entries = true;
        repeated_entry_rows = 0;
      }
      uint16_t *output = outputs[screen_y & (DITHER_ROW_PERIOD - 1)];
      if (repeated_entry_rows < DITHER_ROW_PERIOD) {
        uint32_t dither_pattern = 0;
        for (int column = 0; column < 4; ++column)
          dither_pattern |=
            (uint32_t)(dither[((screen_y & 3) << 2) | ((screen_x + column) & 3)] *
                       8 / 16)
            << (column * 8);
        for (int word = 0; word < (end - start + 3) / 4; ++word)
          dither_words[word] = dither_pattern;
        if (core_shifts_green_and_narrows_blue)
          filter_row<Transparent, true>(
            entries, end - start, (screen_x % 3 + 3) % 3,
            reinterpret_cast<const uint8_t *>(dither_words), output
          );
        else
          filter_row<Transparent, false>(
            entries, end - start, (screen_x % 3 + 3) % 3,
            reinterpret_cast<const uint8_t *>(dither_words), output
          );
      }
      if (!Transparent) {
        // Rows filtered while the previous row is still being sent wait in
        // outputs, so a light row makes up for a heavy one.
        while (unsent_row <= row && !display->dmaBusy()) {
          push_output_row(
            display, outputs, screen_x, y + unsent_row, end - start
          );
          ++unsent_row;
        }
        continue;
      }
      int run_start = 0;
      for (int column = 0; column <= end - start; ++column) {
        if (column < end - start &&
            !(entries[column + 1].color & COLOR_TRANSPARENT_BIT))
          continue;
        if (column > run_start)
          display->pushImage(
            screen_x + run_start, screen_y, column - run_start, 1,
            reinterpret_cast<const lgfx::swap565_t *>(output + run_start)
          );
        run_start = column + 1;
      }
    }
    for (; !Transparent && unsent_row < end_row; ++unsent_row)
      push_output_row(display, outputs, screen_x, y + unsent_row, end - start);
  }
}

void
area512_quaternary_crt_push(
  lgfx::LGFX_Device *display,
  lgfx::LGFX_Sprite *sprite,
  int x,
  int y,
  bool core_shifts_green_and_narrows_blue,
  bool transparent,
  uint32_t transparent_color_index
) {
  if (display == nullptr || sprite == nullptr)
    return;
  const int width = sprite->width();
  const int height = sprite->height();
  if (width <= 0 || height <= 0)
    return;
  const auto *pixels = static_cast<const uint8_t *>(sprite->getBuffer());
  if (pixels == nullptr)
    return;
  const PaletteTables palette_tables =
    build_palette_tables(sprite->getPalette());
  const int first_row = y < 0 ? -y : 0;
  const int end_row =
    height < display->height() - y ? height : display->height() - y;
  // pushImage copies rows shorter than 1024 bytes into the bus buffer before
  // DMA, so the next row is filtered while this one is still being sent.
  display->startWrite();
  if (transparent)
    push_rows<true>(
      display, palette_tables, pixels, width, height, x, y,
      core_shifts_green_and_narrows_blue, transparent_color_index & 0x0f,
      first_row, end_row
    );
  else
    push_rows<false>(
      display, palette_tables, pixels, width, height, x, y,
      core_shifts_green_and_narrows_blue, transparent_color_index & 0x0f,
      first_row, end_row
    );
  display->endWrite();
}
