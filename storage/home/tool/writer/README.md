# Writer

Small word processor for Area512/Cardputer. Documents are saved as
`.txt` files in `/data/writer`.

## Controls

- Type to insert text; Enter starts a new line; Backspace deletes
- Arrow keys: move the cursor
- `Ctrl+Space`: toggle JP / EN input
- `Ctrl+K` / `Ctrl+J`: page up / page down
- `Ctrl+S`: save the current document; a new document asks for a file name
- `Ctrl+O`: choose a document in `/data/writer` to open (replaces the current document)
- `Ctrl+Q`: quit
- Esc: show the help line

## Notes

- Save and open need a microSD card; it is mounted while the app runs.
- Quitting does not save automatically — press `Ctrl+S` first.
- The open list shows the first 128 documents in name order.
- An existing `/data/writer.txt` moves to `/data/writer/writer.txt` on start.
