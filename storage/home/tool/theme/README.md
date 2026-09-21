# Theme

Theme color editor for Area512/Cardputer. Edits `Area512_data/etc/theme`
on the microSD card.

Rows: `background`, `text`, `emphasis`, `border`, `selected`, `box`, `image`.

## Controls

- `j` / `k`: select a row
- `h` / `l`: select the R / G / B channel
- `+` / `-`: channel value +1 / -1
- `]` / `[`: channel value +16 / -16
- Enter: type a color as `0xRRGGBB`, or pick a background image on the
  `image` row
- Tab: preview the theme (Tab or Esc returns)
- `s`: save to `Area512_data/etc/theme`
- Esc: quit (asks to discard unsaved changes)

## Notes

- Background images are the `.rgb565` files in `Area512_data/share/backgrounds`.
- The saved theme is applied after a reboot.
