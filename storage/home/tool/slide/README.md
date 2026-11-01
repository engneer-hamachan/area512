# Slide

Markdown slide viewer for Area512/Cardputer.
Each project is a directory in `/data/slide`, and each page is a numbered file
(`1.md`, `2.md`, ...) in that directory, shown in number order.

```text
/data/slide/talk/1.md
/data/slide/talk/2.md
/data/slide/lesson/1.md
```

The app lists the projects at startup.

## Controls

- Up / Down or `k` / `j`: move in the project list
- Enter: open the selected project
- Esc in the project list: quit
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
- With no project directory, the list shows `No projects`.
- With no numbered `.md` file in the project, the app shows
  `No numbered .md files`.
