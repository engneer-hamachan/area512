# Calc

Tiny spreadsheet for Area512/Cardputer. 26x26 cells (`A1`..`Z26`).

## Controls

- `h` / `j` / `k` / `l`: move the cell cursor
- `e` / Enter: edit the current cell (Enter commits, Esc cancels, Backspace deletes)
- `c`: clear the current cell
- `g`: go to a cell by name (e.g. `B12`; Enter confirms, Esc cancels)
- `s`: save the sheet to `Area512_data/sheet.txt` on the microSD card
- `o`: load `Area512_data/sheet.txt` from the microSD card (replaces the current sheet)
- `q`: quit

## Formulas

A cell starting with `=` is a formula. Anything else is plain text.

- Operators: `+` `-` `*` `/` and parentheses; decimal numbers are allowed
- Cell references: `=A1+B2`
- Range functions: `SUM`, `AVG` (or `AVERAGE`), `MIN`, `MAX`, `COUNT`
  over a cell or a range, e.g. `=SUM(A1:B3)`
- A formula that cannot be evaluated shows `#ERR`
- Division by zero evaluates to 0

## Notes

- Save and open (`s` / `o`) need a microSD card; it is mounted while the app runs.
