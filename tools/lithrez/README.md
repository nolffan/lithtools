# lithrez and LithRez Studio

Cross-platform tools for LithTech resource files (`.rez`), in C++17 with CMake;
they build with clang, gcc or MSVC on Linux, macOS and Windows.

- **`lithrez`**: the command line tool. Its classic commands behave and
  print like Monolith's LITHREZ (1.00 to 1.10), and the archives it creates
  are byte-identical to theirs. Extra commands cover debugging and build
  scripts.
- **LithRez Studio** (`lithrez-studio`, Qt 6): a successor to WinRez LT Studio. It
  has an Explorer-style rez editor, a DTX texture viewer with thumbnails, a
  model preview for `.abc` (NOLF, AvP2) and `.ltb` (NOLF2, Contract
  J.A.C.K., Tron 2.0) models, and a string table editor for `CRes.dll`.
- **`core/`**: the shared library (`lithtech::rez`, `lithtech::dtx`,
  `lithtech::abc`, `lithtech::ltb`, `lithtech::pe`).

`docs/REZ.md` describes the format and how these tools were checked against
the originals.

## Building

```
cmake -S tools/lithrez -B build -G Ninja          # -DLITHREZ_BUILD_GUI=OFF without Qt
cmake --build build
ctest --test-dir build
```

LithRez Studio needs Qt 6.2 or later (Widgets, OpenGL, OpenGLWidgets) and
OpenGL 3.3. Without Qt only `lithrez` and the library are built.

`docker/Dockerfile` is the Linux build environment used here (clang, CMake,
Ninja, Qt 6):

```
docker build -t lithtools-rez-dev tools/lithrez/docker
docker run --rm -u $(id -u):$(id -g) -v $PWD:/src -w /src lithtools-rez-dev \
    sh -c 'cmake -S tools/lithrez -B out/lithrez-build -G Ninja && cmake --build out/lithrez-build'
```

`.github/workflows/tools.yml` builds, tests and packs on all three platforms: a release for every `v*` tag and a nightly for every push to master.

## lithrez

The classic commands work as before, with options as letters in the command:

```
lithrez cv game.rez dir             create (verbose) from a directory tree
lithrez c game.rez dir "*.dtx;*.ltb" only these extensions (LithRez 1.10)
lithrez v game.rez                  view
lithrez x game.rez outdir           extract
lithrez f game.rez dir              freshen: add or replace files
lithrez s game.rez                  sort/compact (keeps game.BAK)
lithrez i game.rez                  is-sorted information
```

Options are `v` (verbose), `z` (warn about zero-length files) and `l` (keep
the case of names).

Extra commands: `list` (`--long`, `--json`), `header`, `dump`, `verify`,
`cat`, `get`, `add`, `rm`, `mkdir`, `diff`, `roundtrip`. Run `lithrez help`
for details.

### In CI

The original tools stamp the clock on every entry, so two builds never
match. Give a fixed time to get the same bytes every time:

```
lithrez c build/mymod.rez mod --time 1700000000
SOURCE_DATE_EPOCH=$(git log -1 --format=%ct) lithrez c build/mymod.rez mod
lithrez verify build/mymod.rez
```

`--compat 1.10` matches the later LithRez builds (NOLF2, Contract
J.A.C.K., Tron 2.0), which write 0 for every resource ID. The default,
1.00, matches the NOLF and AvP2 tools, which number them. `--title ""`
leaves the header's second line blank, as Shogo and Blood 2 files have it.

On Linux and macOS the directory is listed in Windows' (NTFS) order, so the
archive is the same as one built on Windows.

## LithRez Studio

- **Browse:** folder tree, location bar with history, Back/Forward/Up, and
  large icon, small icon, list and details views. Textures get thumbnails.
- **Edit:** insert files or folders (also by dragging them in), insert empty
  folders, rename, delete, and cut/copy/paste. Paste moves, as in WinRez: a
  rez file never holds the same data twice.
- **Extract:** the selection or everything, or drag items out to your file
  manager.
- **Make:** a new empty rez, a "Rez Project" from a folder (the same bytes
  as `lithrez c`), the Rez Wizard with WinRez's per-game folder layouts,
  Import (another rez's contents) and Merge (several rez files into one).
- **Preview pane:** textures, models (`.abc` and `.ltb`) with their skins
  and animations, text, and hex for everything else. For NOLF2, Contract
  J.A.C.K. and Tron 2.0, skins come from the archive's own
  `ATTRIBUTES/MODELBUTES.TXT` when it has one. Double-click opens the texture viewer, the
  model viewer, or (for a DLL) the string table editor; other files open
  with their system program.
- **Settings:** case handling, compatibility (resource IDs), and the header
  title for new files. File > Rez File Information shows the header and lets
  you edit its title.
- **Tools menu:** the texture viewer, model viewer and string editor, which
  also work on loose files.

Command line, with WinRez LT's switches:

```
lithrez-studio [file.rez | file.dtx | file.dll]
lithrez-studio name.rez /C <files or folders>...   create name.rez from them
lithrez-studio /A <files or folders>...            add them, then ask for a name
lithrez-studio /X name.rez <folder>                extract everything
lithrez-studio /E name.rez                         extract, asking where
```

WinRez's Explorer integration (shell menus and DTX thumbnails in Explorer)
is left to the desktop: point a file association or a context-menu entry at
these switches.
