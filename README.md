# simple-md

`simple-md` is a small, read-only Markdown viewer for the terminal. It renders
documents with colour and text attributes in an alternate-screen pager, then
restores the terminal when it exits.

When output is redirected, it writes rendered plain text without terminal
control sequences and exits without waiting for input.

It is implemented in C and has no runtime dependencies beyond a POSIX-like
system and a UTF-8 terminal with 256-colour support.

## Build

```sh
make
```

The resulting executable is `./simple-md`. CI is configured to build with both
GCC and Clang.

To install it:

```sh
make install PREFIX="$HOME/.local"
```

`PREFIX` defaults to `/usr/local`; staged packaging is supported through
`DESTDIR`.

## Usage

```sh
simple-md README.md
simple-md < README.md
cat README.md | simple-md
simple-md --width=100 document.md
```

Options:

```text
--width=COLUMNS  Override the detected terminal width (1–10000)
--pager=MODE     Select auto, always, or never (default: auto)
--no-color       Suppress colours and text attributes
-h, --help       Show usage
-V, --version    Show the version
```

Navigation:

| Key | Action |
|---|---|
| Up / Down | Scroll one line |
| Page Up / Page Down | Scroll one page |
| Home / End | Jump to the beginning or end |
| Left / Right | Pan wide tables and code blocks |
| `h` / `?` | Show help |
| `q` / Escape | Quit |

In automatic mode, documents that fit in the terminal are printed directly;
longer documents open in the interactive pager. Use `--pager=always` when you
want interactive features for a short document, or `--pager=never` to retain
the rendered output in terminal scrollback.

Setting the [`NO_COLOR`](https://no-color.org/) environment variable has the
same effect as `--no-color`. Pager navigation continues to use terminal control
sequences; redirected output never contains them.

## Markdown support

- Headings through level four
- Paragraph reflow and hard line breaks
- Bold, italic, bold italic, and inline code
- Fenced code blocks
- Syntax highlighting for C, C++, JavaScript, TypeScript, Python, and Pascal
- GFM-style pipe tables with alignment and Unicode borders
- Ordered and unordered lists, nested to three levels
- Blockquotes, links, and horizontal rules
- CJK and emoji display widths
- Bidirectional text rendering for RTL scripts

This is a deliberately focused Markdown parser, not a complete CommonMark
implementation. Unsupported constructs are rendered as ordinary text.

## Tests

```sh
make test       # unit suite plus real-PTY CLI checks
make sanitize   # unit tests under AddressSanitizer and UBSan
```

The terminal test starts the actual executable in a pseudo-terminal and checks
both `q` and Escape exits, including alternate-screen entry and restoration.
Python 3 is needed only for this PTY test.

## Architecture

```text
input → parser → AST → styled layout → viewport → terminal
```

- `md_parse` and `md_ast` parse block and inline elements.
- `md_render`, `md_table`, and `md_highlight` build styled display lines.
- `md_viewport` handles scrolling, clipping, panning, and BiDi ordering.
- `md_terminal` owns raw mode, signal handling, terminal sizing, and key input.
- `term_style`, `unicode_width`, and `bidi` are bundled support modules.

The extraction history and its reproducible checks are documented in
[`docs/EXTRACTION.md`](docs/EXTRACTION.md).

## Origin and license

`simple-md` was extracted from
[`SonicField/nbs-framework`](https://github.com/SonicField/nbs-framework).
The filtered Git history retains the original authorship and commit messages.

Released under the MIT License. See [`LICENSE`](LICENSE).
