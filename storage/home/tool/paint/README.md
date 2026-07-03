# Paint

Pixel paint for Area512/Cardputer. 120x60 canvas (2px cells), 8-color palette.

## Controls

- `h` / `j` / `k` / `l`: move cursor left / down / up / right
- `y` / `u` / `b` / `n`: move diagonally
- `space`: paint at the cursor; pressing it at another spot draws a line
  from the last painted point
- `p`: pen down / up — while the pen is down, moving the cursor draws
- `c`: cycle the palette (white, red, gold, green, cyan, blue, pink, black)
- `e`: erase at the cursor
- `+` / `=`: bigger brush (up to 8)
- `-`: smaller brush (down to 1)
- `x`: clear the canvas
- `q`: quit

## Notes

- Cursor movement steps by the brush size.
- There is no save; the canvas is gone when you quit.
