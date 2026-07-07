# Solitair

Tiny Klondike solitaire for Area512/Cardputer.

## Screen

- `ST`: stock
- `WA`: waste
- `F1`..`F4`: foundations. These are screen labels, not keyboard keys.
- `1`..`7`: tableau columns

## Controls

- `h` / `l`: move cursor
- `1`..`7`: jump to that tableau column — selects it, or moves the
  selected card there
- `0`: jump to the waste, selecting it when nothing is selected
- `d`: draw from stock, or reset stock when it is empty
- `space` / Enter: select a card, or move the selected card to the cursor
- `k`: scroll the current tableau column up, or widen a selected stack
- `j`: scroll the current tableau column down, or narrow a selected stack
- `f`: move the selected card to its foundation, or cycle the cursor
  through `F1`..`F4`
- `a`: auto-move one available card to a foundation
- `n`: start a new game
- `q`: quit

To move a card to a foundation, press `f` with a card selected, or move
the cursor to `F1`..`F4` and press `space` or Enter.

The high score is kept in `Area512_data/data/solitair.txt` on the microSD card when one is
inserted.
