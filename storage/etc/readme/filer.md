# Console

Manage files, run apps, and work in the terminal on Area512.

Use the file list to select files and directories, or press `t` to enter terminal commands. Keys are case-sensitive.

## Navigation

- `j` / Down / `.`: select the next entry
- `k` / Up / `;`: select the previous entry
- Enter / Right / `/`: open the selected entry
- `u` / Backspace / Left / `,`: go to the parent directory
- `1` - `9`: select one of the first nine entries in the visible list
- `R`: run the app in the selected directory

Enter opens a directory. To launch its app instead, select the directory and press uppercase `R`.

## Open and run

Enter chooses an action from the selected file's extension.

- `.rb`: compile to `.mrb`, then run
- `.mrb`: run Ruby bytecode
- `.py`: compile to `.mpy`, then run
- `.mpy`: run Python bytecode
- `.md`: open the Markdown viewer
- `.a5d`: open the dot image editor

For other files, use `e` to open Vim. Vim editing is not available for `.mrb`, `.mpy`, or `.a5d` files.

## Directory apps

`R` checks the selected directory in this order:

1. `main.manifest`
2. `main.mrb`
3. `main.mpy`

Filer runs the first one it finds. This action does not compile source files first. Use `c` or `a` to compile sources when needed.

On an external-display setup, Filer shows the app's `README.md` on the internal display when launching the app.

## Compile

- `c`: compile the selected `.rb` or `.py` file
- `a`: compile all `.rb` and `.py` files directly in the current directory

Compilation writes `.mrb` or `.mpy` files beside the sources. `a` does not descend into subdirectories and stops at the first error.

## Files

- `e`: edit the selected file in Vim
- `N`: create an empty file in the current directory
- `K`: create a directory in the current directory
- `m`: move or rename the selected entry
- `C`: copy the selected entry
- `x`: delete the selected entry after confirmation

In a name or destination prompt, Enter confirms, Backspace deletes the last character, and Esc cancels. Enter with empty input also cancels.

## Move and copy

Select an entry, then press `m` to move or rename it, or uppercase `C` to copy it. Enter a destination relative to the current directory or an absolute path starting with `/`.

- An existing destination directory receives the entry under its original name.
- Otherwise, the destination specifies the new name. Its parent directory must already exist.
- An existing file or same-named entry is not overwritten.
- Copying a directory includes its contents and subdirectories.

## Delete

`x` asks for confirmation before deleting the selected entry. Press `y` or `Y` to delete; press `n`, `N`, or Esc to cancel.

Deleting a directory also deletes its contents and subdirectories.

## Terminal

Press `t` in the file list to open the terminal. Type a command and press Enter. Use `exit` to return to the file list.

The terminal and file list share the current directory. After a terminal `cd`, `exit` returns to the file list in that directory.

The command forms below use `path`, `file`, `dir`, `src`, `dst`, and `name` as placeholders; replace them with the required names or paths.

## Terminal navigation

- `pwd`: show the current directory
- `ls`: list the current directory
- `ls dir`: list another directory
- `cd path`: change directory
- `cd`: return to `/home`
- `help`: list available command names
- `clear`: clear terminal output
- `exit`: return to the file list

Paths can be relative to the current directory or absolute. `.` means the current directory and `..` means its parent. Terminal arguments support `~` and `~/` for `/home`.

Command arguments are separated by spaces. Quoting and escaping spaces in names are not supported.

## Terminal run and edit

- `run file`: run a `.rb`, `.mrb`, `.py`, or `.mpy` file
- `run dir`: launch a directory app using the same order as `R`
- `md file`: view a `.md` file
- `dot file`: edit an `.a5d` image
- `vim file`: edit a file in Vim
- `compile file`: compile a `.rb` or `.py` file
- `compile --all`: compile sources directly in the current directory
- `compile --all dir`: compile sources directly in the specified directory

`run` compiles `.rb` and `.py` files before execution. `vim` can open a new file when its parent directory exists; `.mrb`, `.mpy`, and `.a5d` are excluded.

## Terminal file operations

- `touch name`: create an empty file
- `mkdir name`: create a directory
- `mv src dst`: move or rename an entry
- `cp src dst`: copy an entry
- `rm path`: delete a file or directory after confirmation

`touch` reports an error if the file already exists. `mv` and `cp` use the same destination rules as `m` and `C`. `rm` includes directory contents and uses the same confirmation keys as `x`.

## Terminal input

- Left / Right: move the input cursor
- Backspace: delete the character before the cursor
- Up / Down: recall command history
- Tab: complete a command or path; press again to cycle through matches
- Ctrl-F: append the displayed suggestion
- Esc: clear the input line
- Enter: execute the command

## REPL and status

- `irb`: start the Ruby REPL
- `python-repl`: start the Python REPL
- `top`: show battery, VM memory, RAM, and stack percentages
- `reboot`: restart the device

## Other

- `r` in the file list: restart the device
