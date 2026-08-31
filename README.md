# ｍａｄｑｔ

A read-only markdown viewer for the X Window System written in Rust for safety, Qt to not suck.

no editing, no menu bar, no distractions — open a markdown file, read it,
rendered properly with syntax highlighting, diagrams, and math.

## FEATURES

 * read-only markdown rendering via `QWebEngineView`
 * toolbar-first UI per the KDE HIG: no menu bar, primary actions with text
   labels on a fixed toolbar, secondary actions in a hamburger menu
 * multiple windows, each fully independent
 * drag and drop support
 * overlay find bar with find-as-you-type that floats over the document
   instead of reflowing it; the field tints red on no match
 * table of contents overlay: click or press enter to jump to a section;
   headings get stable anchor ids automatically
 * links always open externally (anchors within the document still scroll
   in place); right-click menu with copy, copy link address, and select all
 * syntax highlighting for fenced code blocks (via `syntect`), with colors
   drawn from the active theme; unknown/unlabeled blocks render as plain code
 * mermaid diagrams render to themed SVG via a bundled mermaid.js, loaded
   only for documents that use it
 * LaTeX math, inline and display, via a bundled KaTeX, also loaded on demand
 * GitHub-style alerts (note/tip/important/warning/caution) with colored
   rules, icons, and titles; definition lists
 * scroll position is preserved across live reloads and appearance changes,
   so edits made while you read don't snap you back to the top
 * named themes (fonts, layout, colors): built-in Light, Dark, and Sepia,
   plus user themes you create by duplicating and editing, with a live
   preview while editing
 * relative assets resolve from the markdown file's directory
 * bundled OFL fonts so rendering looks right regardless of installed
   system fonts

## REQUIREMENTS

 * Linux
 * CMake 3.22 or newer
 * Qt 6 with `Widgets` and `WebEngineWidgets`
 * Rust toolchain with Cargo
 * a C++20-capable compiler

## BUILDING

```
cmake -S . -B build
cmake --build build
```

first configure fetches Corrosion from GitHub. `compile_commands.json` is
emitted in `build/`.

### install

```
sudo cmake --install build --prefix /usr/local
```

installs the binary, a desktop entry associated with `text/markdown`, and a
scalable hicolor icon, then refreshes the desktop and icon caches. to make
madqt the default handler for markdown files:

```
xdg-mime default madqt.desktop text/markdown
```

### flatpak

```
cmake -B build .
cmake --build build --target flatpak
flatpak run org.xeyes.madqt
```

requires `flatpak-builder`.

## RUNNING

```
./build/madqt
./build/madqt /path/to/file.md
```

a feature tour lives in `demo/demo.md`:

```
./build/madqt demo/demo.md
```

## KEYBOARD SHORTCUTS

 * `CTRL + O` - open a file
 * `CTRL + N` - open a new window
 * `CTRL + SHIFT + O` - open a file in a new window
 * `CTRL + F` - find; `F3` / `SHIFT + F3` - next/previous match, `ESC` - close
 * `CTRL + T` - table of contents
 * `CTRL + =` / `CTRL + -` / `CTRL + 0` - zoom in/out/reset
 * `F10` - hamburger menu
 * `CTRL + Q` - quit

## STYLING

rendering style is generated in `src/MarkdownAppearance.cpp` as CSS custom
properties (user-tunable tokens) plus a static stylesheet. bundled fonts are
registered with `QFontDatabase` for the Qt font dialog and served to the page
through the custom CORS-enabled `madqt:` scheme (`src/ResourceSchemeHandler.cpp`),
because WebEngine cannot see Qt application fonts and blocks `@font-face` over
`qrc:`.

## TROUBLESHOOTING

### THE MARKDOWN LOOKS DIFFERENT THAN IT DOES ON GITHUB
madqt renders CommonMark plus GitHub-style alerts, Mermaid, and KaTeX. if
something still looks wrong, check `demo/demo.md`, which exists specifically
to break renderers.

### I CAN'T EDIT THE FILE
that's the "read-only" part of "read-only markdown viewer." open a text editor.

### I FOUND A BUG
nice.

### DOES THIS WORK ON WINDOWS
the software was written for Linux but some users on the discord have had
luck [getting it running on Windows](https://goatse.cx/).

## ACKNOWLEDGEMENTS

madqt uses [pulldown-cmark](https://github.com/raphlinus/pulldown-cmark) to
turn markdown into HTML, [syntect](https://github.com/trishume/syntect) to
make code blocks look like someone cared, bundled
[mermaid](https://mermaid.js.org/) and [KaTeX](https://katex.org/) for
diagrams and math, and [Corrosion](https://github.com/corrosion-rs/corrosion)
to keep Cargo and CMake from fighting. fonts are bundled from the IBM Plex,
Source Sans/Serif, and JetBrains Mono OFL families so text doesn't depend on
whatever happens to be installed.

## LICENSE

Copyright (C) 2026 laplaces-agent.

GPL-3.0-or-later. the full text is in [LICENSE](LICENSE), all 674 lines of it.
the short version: it's yours, do what you like, just don't take it away from
anyone else.
