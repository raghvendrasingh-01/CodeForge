# CodeForge: Nano-Style Terminal Text Editor

CodeForge is a small Nano-style, always-editing terminal editor written in
C++17 for Linux. It uses
POSIX `termios` and ANSI escape sequences for the terminal boundary, while the
editor, buffer, modes, commands, and tests use only the standard library.

## Build and run

```sh
make
make test
./codeforge path/to/file.txt
./codeforge                 # start an unnamed, empty document
make run FILE=path/to/file.txt
make clean
```

The Makefile deliberately lists library sources separately from `main.cpp` and
the test programs. It also emits dependency files, so changing a header
rebuilds the affected objects. The normal flags are `-std=c++17 -Wall -Wextra
-Wpedantic -O2`; the code is also checked during development with conversion,
sign-conversion, and shadow warnings enabled.

## Controls

There is no mode switch for ordinary editing: printable characters, Enter,
Tab, Backspace, Delete, and arrow keys work immediately.

`M-` is Meta, which on most terminals is Alt, or Escape pressed before the key.

| Shortcut | Action |
|---|---|
| Ctrl-O | Write Out; edit the filename and press Enter |
| Ctrl-S | Save to the current filename without asking |
| Ctrl-X | Exit; choose `y` to save, `n` to discard, or `c` to cancel |
| Ctrl-W, Ctrl-F | Where Is; type a pattern and press Enter |
| M-W | Repeat the last search |
| Ctrl-\ | Replace; give a pattern, then its replacement |
| Ctrl-_, Ctrl-/ | Go To Line; accepts `line` or `line,column` |
| Ctrl-K | Cut the current line; repeat to collect several |
| Ctrl-U | Paste the whole cut buffer |
| M-U, M-E | Undo, Redo |
| Ctrl-Z, Ctrl-R | Undo, Redo -- an extension, see below |
| M-N | Turn line numbers on or off |
| Ctrl-L | Refresh the screen |
| Ctrl-A, Ctrl-E | Start, end of line |
| Ctrl-P, Ctrl-N | Previous, next line |
| Ctrl-Y, Ctrl-V | Page up, page down |
| Ctrl-D | Forward delete, the mirror image of Backspace |
| Ctrl-G | Show a one-line shortcut reminder |
| Ctrl-C | Report the cursor position, or cancel an open prompt |

Inside a prompt, Ctrl-C cancels, Enter accepts, and Backspace, Delete, and the
left and right arrows edit the answer a whole character at a time.

The older Vim-style modes remain in the source as an OOP comparison, but the
active application path is the Nano-style `NANO` mode. Characters such as
`h`, `j`, `k`, `l`, `:`, and `/` are ordinary text when typed.

## Text encoding

The editor treats a UTF-8 character, not a byte, as the unit the user works
with. Backspace, Delete, and the left and right arrows move over and remove a
whole character, so an accented letter cannot be cut in half and leave a broken
fragment behind. Screen width is measured in columns rather than bytes, which
matters because a line of accented text is about twice as many bytes as columns:
measuring it with `string::size` would fill half the window and slicing it with
`substr` could emit half a character. Tabs expand to eight-column tab stops and
count as one character but eight columns, which is the distinction Ctrl-C's
report makes. Control bytes are shown in caret notation so file contents cannot
inject ANSI sequences into the renderer.

## Nano compatibility

The behaviour was compared against GNU nano 8.7.1 by driving both programs
through a pseudo-terminal and diffing the resulting screens and saved files.
These agree exactly, character for character: the status and shortcut rows; the
title row apart from the program name itself, down to the column the file name
is centred on and where the `Modified` marker sits at three window widths; the
prompt labels and the highlighting around them; `Read`/`Wrote N
line(s)`; `Cutbuffer is empty`; the Ctrl-C position report including its column
alignment; the `Undid`/`Redid addition`, `deletion`, `line break`, `line join`,
`cut`, and `paste` messages; and horizontal scrolling, down to which column the
window jumps to, where the cursor lands after the jump, and the `<` and `>`
markers on a line that runs off an edge.

Four differences are deliberate.

* Ctrl-Z is Undo and Ctrl-R is Redo, in addition to nano's M-U and M-E. Nano
  reserves Ctrl-Z for job control and answers it with a hint; because this
  editor does not implement suspend, the key would otherwise do nothing.
* Ctrl-G prints a one-line reminder where nano opens a scrollable help viewer.
* The shortcut rows advertise only keys this editor actually implements, so
  nano's `^R Read File`, `^T Execute`, and `^J Justify` are absent and the
  remaining entries are arranged to fill the two rows.
* A frame is one `write()` of the whole screen, where nano sends just the
  characters that changed. The cost is about two kilobytes per keystroke
  instead of twenty bytes; the benefit is that a half-drawn frame can never be
  seen, which is why `docs/DESIGN.md` fixes it as a design rule.

Two smaller differences follow from the buffer invariant that there is always at
least one line, which `docs/DESIGN.md` fixes as a rule. Saving an emptied buffer
writes a single newline where nano writes nothing. And nano keeps a phantom
empty line after the last one, so its cursor can step below the final character
and a Backspace there merely rejoins the line; here the cursor stops at the end
of the text, where Backspace deletes the character before it.

## Architecture in one paragraph

`Editor` owns a `Terminal`, `Buffer`, persistent mode objects, and undo and
redo stacks of `unique_ptr<EditCommand>`. Each key is read once, dispatched
through the active Nano policy, translated into an edit command when appropriate,
executed against the encapsulated buffer, and followed by one complete frame
render. Modes are persistent identities; changing mode only changes a non-owning
pointer, so a mode cannot destroy itself while its virtual call is still on the
stack.

## Documentation map

* [docs/CODE-TOUR.md](docs/CODE-TOUR.md) is a guided walk through every file and
  the code inside it, ordered so each part only uses what came before. Start here
  to learn the codebase.
* [docs/DESIGN.md](docs/DESIGN.md) explains ownership, dispatch, rendering, and
  error boundaries.
* [docs/OOP-CONCEPTS.md](docs/OOP-CONCEPTS.md) maps course concepts to source.
* [docs/STUDY-ORDER.md](docs/STUDY-ORDER.md) gives a reading sequence.
* [docs/VIVA.md](docs/VIVA.md) contains difficult oral-exam questions and
  defensible answers.
* [docs/superpowers/specs/2026-08-20-modal-terminal-editor-design.md](docs/superpowers/specs/2026-08-20-modal-terminal-editor-design.md)
  is the historical, superseded modal design record. It is useful for studying
  the retained legacy classes, but `README.md` and `docs/DESIGN.md` define the
  current Nano workflow.

## Deliberate simplifications

The buffer uses `vector<string>`, so inserting in the middle of a long line is
O(line length). A gap buffer or piece table would improve that workload but
would add machinery unrelated to this OOP exercise. The editor also uses one
undo command per editing keystroke; it does not invent a compound command just
to group a typing session. The buffer compares its current lines and trailing-
newline state with the last loaded/saved snapshot, which is O(file size) per
call rather than a stored dirty flag, while `Editor` history state IDs keep
undoing back to that saved state clean. Operator overloading, friend
functions, templates, and mutable static data do not fit naturally here, so the project documents
their absence instead of adding artificial examples.

## Manual edge cases

Try an empty file, a one-line file, a very long line, a file without a trailing
newline, tabs, CRLF/control bytes, a rapid terminal resize, Backspace at column
zero, undo past the beginning, and redo after making a new edit. Try Ctrl-K on
the only line, then Ctrl-Z, and verify that undo restores the original text.
Try an accented character and confirm that one Backspace removes all of it, and
a line of them that is wider than the window and confirm that scrolling
sideways never shows a replacement character. On a line wider than the window,
check that `>` appears in the last column, that walking right past the edge
moves the view a chunk at a time rather than a column, and that the shorter
lines around it stay put.
Repeat Ctrl-K followed by Ctrl-U and verify that the buffer never has zero
lines.
