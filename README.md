# AREA512

<p align="center">
  <img src="docs/assets/images/emblem-transparent.webp" alt="AREA512" width="480" />
</p>

Welcome to AREA512!

AREA512 is an OS built for the Cardputer ADV and Cardputer v1.1,
a tiny device with just 512KB of RAM and 8MB of flash storage!

It is based on FemtoRuby and includes MicroPython,
so you can write Ruby and Python right on the Cardputer,
then compile and run them — all on the device!

[Website](https://engneer-hamachan.github.io/area512/) · [Installation](https://engneer-hamachan.github.io/area512/install.html) · [Manual](https://engneer-hamachan.github.io/area512/manual.html)

## Supported Devices

AREA512 supports Cardputer ADV and Cardputer v1.1, either on their own or with the TERM512 external display and case.

> [!TIP]
> Build the external display and case: **[TERM512 on GitHub →](https://github.com/Prokuon/term512)**
> Then flash the binary for your display in [Quick Install](#quick-install).

<table align="center">
  <tr>
    <th>Cardputer</th>
    <th>Cardputer + TERM512</th>
  </tr>
  <tr>
    <td><img src="image/device.jpg" alt="AREA512 on a Cardputer ADV" height="226" /></td>
    <td><img src="image/device_ext.jpg" alt="AREA512 on a Cardputer ADV in a TERM512 case with its external display" height="320" /></td>
  </tr>
</table>

## Applications

![Solitair, one of the built-in apps, running on the device](image/solitair_ui.jpg)

Write documents with Writer, organize your days with Scheduler, work with
spreadsheets in Calc, or draw with Paint. Take a break with Solitair, Bomb,
and Space Lander, a lunar landing game written in Python.

Apps come preinstalled under `/home/tool` and `/home/game`, with a README in
each app directory. Slide, Gallery, Theme, and the Dot editor provide tools
for presentations, exploring widgets, and customizing your device.

[Explore the applications](https://engneer-hamachan.github.io/area512/applications.html) · [App controls](https://engneer-hamachan.github.io/area512/manual.html#applications)

## On-device Programming

![Editing Ruby code in the on-device vim](image/edutor_ui.jpg)

Write Ruby and Python right on the Cardputer. The built-in vim-style editor
provides syntax highlighting, automatic indentation, code completion,
diagnostics, and hover information. Edit, compile, and run your code on the
device, or try ideas interactively in the Ruby and MicroPython REPLs.

Built-in APIs cover graphics, sprites, widgets, files, and hardware access.
The preinstalled apps are examples you can read and build on.

[Programming guide](https://engneer-hamachan.github.io/area512/programming.html) · [Ruby API](https://engneer-hamachan.github.io/area512/ruby-api.html) · [MicroPython API](https://engneer-hamachan.github.io/area512/python-api.html) · [Widget API](https://engneer-hamachan.github.io/area512/widget-api.html)

## Quick Install

All you need is esptool:

```sh
pip install esptool

# Cardputer ADV
esptool.py -c esp32s3 -b 460800 write_flash --flash_mode dio --flash_size 8MB --flash_freq 80m 0x0 firmware/Area512Adv.bin

# Cardputer v1.1
esptool.py -c esp32s3 -b 460800 write_flash --flash_mode dio --flash_size 8MB --flash_freq 80m 0x0 firmware/Area512V11.bin

# 320x240 ST7789 panel
esptool.py -c esp32s3 -b 460800 write_flash --flash_mode dio --flash_size 8MB --flash_freq 80m 0x0 firmware/Area512TFT7789.bin

# 320x240 ILI9341 panel
esptool.py -c esp32s3 -b 460800 write_flash --flash_mode dio --flash_size 8MB --flash_freq 80m 0x0 firmware/Area512TFT9341.bin

# T-Deck Plus
esptool.py -c esp32s3 -b 460800 write_flash --flash_mode dio --flash_size 8MB --flash_freq 80m 0x0 firmware/Area512TDeck.bin
```

- Insert a FAT32-formatted microSD card into the Cardputer (it is used to store app data).

See the [installation guide](https://engneer-hamachan.github.io/area512/install.html) for firmware downloads and setup details.

After startup, use `j` / `k` to select an entry, Enter to open a directory,
and Backspace to go back. Select an app directory under `/home/tool` or
`/home/game` and press uppercase `R` to run it.
See the [operation manual](https://engneer-hamachan.github.io/area512/manual.html) for more controls.

## Updating

After flashing new firmware, open the terminal with `t`, run `fullupdate`,
and answer `y` to update the preinstalled files on the microSD card.
The device reboots when the update finishes.

> [!WARNING]
> Back up your changes first. Firmware-provided files in `/etc` and
> `/share/backgrounds` are overwritten, and each preinstalled app directory
> under `/home/tool` and `/home/game` is deleted and rewritten. This removes
> edits to `etc/theme` and files you added inside those app directories.
> Your own app directories are kept.

[Update details](https://engneer-hamachan.github.io/area512/install.html#updating)

## Building for Contributors

### Requirements

- ESP-IDF v5.5+
- Ruby + Bundler
- M5 Cardputer ADV or Cardputer v1.1
- USB-C cable

### Setup

```sh
git clone --recursive git@github.com:engneer-hamachan/area512.git
cd area512
. $YOUR_ESP_IDF_PATH/export.sh
rake setup
```

If you already cloned without `--recursive`:

```sh
git submodule update --init --recursive
```

### Build and Flash

```sh
# Cardputer ADV build
rake build
rake flash

# Cardputer v1.1 build
rake build:v1.1
idf.py -B build/v11 flash
```

For the 320x240 display with a 240x135 application window:

```sh
# ST7789 panel
rake build:tft7789
idf.py -B build/tft7789 flash

# ILI9341 panel
rake build:tft9341
idf.py -B build/tft9341 flash
```

For the T-Deck Plus:

```sh
rake build:tdeck
idf.py -B build/tdeck flash
```

## T-Deck Plus Keymap

> [!NOTE]
> Key bindings on the T-Deck Plus are tricky, so refer to this map.
> On the Cardputer, keys type what their labels show.

Both Shift keys behave the same. Sym, Alt, and Shift output nothing on their own.

| Key | Plain | Shift | Sym | Sym + Shift | Alt |
| --- | --- | --- | --- | --- | --- |
| `q` | `q` | `Q` | `#` | `=` | Ctrl-Q |
| `w` | `w` | `W` | `1` | Ctrl-Q | Ctrl-W |
| `e` | `e` | `E` | `2` | Ctrl-R | Ctrl-E |
| `r` | `r` | `R` | `3` | Ctrl-S | Ctrl-R |
| `t` | `t` | `T` | `(` | Backspace | Tab |
| `y` | `y` | `Y` | `)` | Tab | Ctrl-Y |
| `u` | `u` | `U` | `_` | `?` | Ctrl-U |
| `i` | `i` | `I` | `-` | Enter | Tab |
| `o` | `o` | `O` | `+` | Ctrl-K | Ctrl-O |
| `p` | `p` | `P` | `@` | Space | Ctrl-P |
| `a` | `a` | `A` | `*` | Ctrl-J | Ctrl-A |
| `s` | `s` | `S` | `4` | Ctrl-T | Ctrl-S |
| `d` | `d` | `D` | `5` | Ctrl-D | Ctrl-D |
| `f` | `f` | `F` | `6` | Ctrl-V | Ctrl-F |
| `g` | `g` | `G` | `/` | `[` | Ctrl-G |
| `h` | `h` | `H` | `:` | `]` | Backspace |
| `j` | `j` | `J` | `;` | Esc | Esc |
| `k` | `k` | `K` | `'` | Ctrl-G | Ctrl-K |
| `l` | `l` | `L` | `"` | Ctrl-B | Ctrl-L |
| `z` | `z` | `Z` | `7` | `<` | `<` |
| `x` | `x` | `X` | `8` | `>` | `>` |
| `c` | `c` | `C` | `9` | Ctrl-C | Ctrl-C |
| `v` | `v` | `V` | `?` | `{` | Ctrl-V |
| `b` | `b` | `B` | `!` | `}` | Ctrl-B |
| `n` | `n` | `N` | `,` | Ctrl-N | Ctrl-N |
| `m` | `m` | `M` | `.` | Ctrl-N | Enter |
| `$` | `$` | Ctrl-D | — | `\|` | — |
| Space | Space | NUL | — | `\|` | NUL |
| Mic | `=` | Ctrl-] | `0` | Ctrl-P | — |
| Enter | Enter | Enter | Enter | Enter | Enter |
| Backspace | Backspace | Backspace | Backspace | Backspace | Backspace |

While Alt is held, Shift and Sym change nothing, except that Space then outputs nothing.

### Customizing the Keymap

On the T-Deck Plus, `/etc/keymap` overrides the map above. List only the keys you want to change, then reboot. Delete the file to go back to the defaults.

```
a=x
sym.a=1
sym.shift.a=2
alt.a=ctrl-a
alt.sym.a=3
```

The left side is any of `shift`, `sym`, and `alt` joined with `.` in any order, followed by the key: a letter, `$`, `space`, `mic`, `enter`, or `backspace`.

| Right side | Output |
| --- | --- |
| One character | That character. `sym.a==` outputs `=` |
| Empty | Nothing |
| `space` `enter` `tab` `esc` `backspace` `delete` | That key |
| `percent` `ampersand` `backslash` `caret` `backtick` `tilde` | `%` `&` `\` `^` `` ` `` `~`, which the T-Deck cannot type |
| `ctrl-a`, `ctrl-@`, `ctrl-backslash` | The control character |

Lines that do not match are ignored, so a line starting with `#` works as a comment. Sym, Alt, Shift, and the trackball cannot be remapped.

| Trackball | Output |
| --- | --- |
| Roll up / down / left / right | Up / Down / Left / Right arrow |
| Press | Enter |

## Contributing

AREA512 welcomes contributions of new apps and AREA512 artwork (splash images and such)!

## Credits

TERM512 was designed and developed by the brilliant creator [Prokuon](https://github.com/Prokuon).

## License

[MIT License](LICENSE)
