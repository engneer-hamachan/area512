# Slide

Markdown slide viewer for Area512/Cardputer.
Each page is a numbered file (`1.md`, `2.md`, ...) in
`Area512_data/home/tool/slide` on the microSD card, shown in number order.

## Controls

- `h` / `l`: previous / next page
- `k` / `j`: scroll the page up / down
- Esc: quit

## Markdown

- `#`, `##`, `###`: headings
- `-`: list item
- `` `code` ``: inline code
- Lines between ` ```ruby ` and ` ``` `: code block

## Notes

- Only files named `<number>.md` are read; other files are ignored.
- With no numbered `.md` file, the app shows `No numbered .md files`.
