# Calc

Tiny spreadsheet for Area512/Cardputer. 26x26 cells (`A1`..`Z26`).

## Controls

- `h` / `j` / `k` / `l`: move the cell cursor
- `e` / Enter: edit the current cell (Enter commits, Esc cancels, Backspace deletes)
- `c`: clear the current cell
- `g`: go to a cell by name (e.g. `B12`; Enter confirms, Esc cancels)
- `s`: save the current sheet; a new sheet asks for a file name
- `o`: choose a sheet in `/data/calc` to open (replaces the current sheet)
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
- Sheets are saved as `.txt` files in `/data/calc`.
- The open list shows the first 128 sheets in name order.
- An existing `/data/sheet.txt` moves to `/data/calc/sheet.txt` on start.
