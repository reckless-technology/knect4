# knect4

Knect 4, against the computer or between two people at one board: drop pucks into a blue upright board, four in a row
across, up or diagonally wins.
A C++20 program with its window drawn in Skia on reckless-toolkit, in the Reckless apps' look.

![The game under way](docs/images/game.png)

## Playing

- Move the pointer over a column and your puck hangs above it, with its landing slot marked; click to drop it. The keys
  work too: 1 to 9 drop into the first nine columns, the left and right arrows move the puck (an amber chevron on the
  frame marks the column the keys hold it over), and Return, space or the down arrow drop it.
- The puck falls behind the frame, showing through the holes as it passes, lands with a click and bounces once or twice.
  The computer's puck sways above the board while it thinks, then glides to its column and drops. A new game lets the
  last one's pucks out: the slider in the frame's foot opens and they fall out column by column with a clatter.
- The panel beside the board sets who you play against (the computer or a friend), the level (Easy, Medium, Hard,
  Perfect), your colour (red or yellow; the computer takes the other), who starts the next game, the board's size
  (6 x 5, 7 x 6, 8 x 7, 9 x 7 or the supersize 16 x 8), the sound and the pucks' marks, and keeps the score. "New game"
  starts again, "Undo" takes back your last move and the computer's answer, and "Hint" shows, after a moment's search,
  the column the engine would play. When a game ends the winning line glows and "Play again" appears over the board.
- The Game menu has the same choices (Ctrl+N a new game, Ctrl+Z undo, Ctrl+H a hint, Ctrl+1 to Ctrl+4 the levels,
  Ctrl+Shift+2 a game against a friend, Ctrl+M the sound); the Board menu the sizes; the View menu the marks and the
  window's size (50 % to 150 %, Ctrl+= and Ctrl+- a size up or down); Help, the About card with the licence and the
  third-party notices.
- Against a friend, two people share the board and the mouse: Red and Yellow take turns, the held puck in the colour of
  whoever is to move, and the panel names them - by the names typed into its Players section, or by colour. "First
  move" chooses Red or Yellow, and "Take turns" passes the first move to the other each new game. Changing the opponent
  starts a new game, and games between two people keep a score of their own.
- Marked pucks carry a cross (red) or a star (yellow) on their faces, for anyone who cannot tell the colours apart.
- Everything chosen, and both scores, are kept between launches in `reckless-technology/knect4/settings.json` in the
  user's settings folder (`%APPDATA%` on Windows, `~/Library/Application Support` on macOS, `~/.config` on Linux).

## The game

`src/game` holds the rules and the computer's play, and needs nothing else.

- `Game` is the board the window shows: the pieces in each slot, the moves, four in a row, a draw and the winning line.
- `BasicPosition` holds a position for the search as two bitboards, a column `rows + 1` bits, in a 64-bit word where
  the board fits (6 x 5, 7 x 6, 8 x 7) or several (`WideBits`: two for 9 x 7, three for 16 x 8). A move is an addition;
  the slots that would make four are found by shifts (`ComputeWinningPositions`).
- `Search` plays on a position: an exact solver after Pascal Pons's design - only moves that do not lose at once, a
  table of upper bounds, moves tried by the threats they make with the centre first, and narrowing null windows - and
  a depth-limited search judged by threats, the rows that favour each side and the centre. Every win scores by how soon
  it comes, so the computer takes a quick win and puts off a loss. Equal moves are chosen between at random, so games
  vary.
  - Easy looks two plies ahead with no judgement, and one move in three looks no further than its own: it takes a four it
    is offered, often stops one, and is otherwise aimless - easy to beat.
  - Medium searches up to six plies on the judgement, content now and then with a move a threat's worth below the best.
  - Hard deepens to twelve plies in two seconds.
  - Perfect solves the position exactly in four seconds where it can - on the standard board from about the eighth
    stone - and otherwise plays the deepest judgement the time allows. On the standard board it opens from a book
    (`src/game/opening_book_data.inc`) that `tools/make_book.cpp` works out with the solver: the best column in every
    position the computer can meet in its first moves while it plays the book's, mirror images folded together, the
    empty board's centre written in from the game's published solution (Allis 1988, Allen 1988). With the book it never
    loses a game it could have saved. The larger boards are too large to solve, so Perfect plays them by search.
- `Engine` puts the five sizes' searches behind one interface the window uses; the hint is Perfect's choice for the side
  to move.

The suites hold the position to the board on hundreds of random games on every size, the solver to an exhaustive search
on late positions of two sizes, and each level to beating the one below it.

## The feel

`src/effects` holds what a drop feels like, written against no toolkit:

- `DropMotion`: the puck falls under gravity from where it was held, hits what it lands on, rises again at a fifth of
  the speed, and comes to rest once a bounce is too slow to rise from, each hit reported with its speed.
- `RenderClick`: the click, made from nothing at the output's rate - a burst of noise a fraction of a millisecond long,
  the puck's own ringing a few kilohertz up, and the board's lower knock beneath (deeper on the base than on another
  puck) - gone in about a tenth of a second, a little different for each seed.
- `ClickMixer`: the clicks played on the audio thread, asked for from the window's through a fixed queue with no lock.

The window plays them through the toolkit's audio device layer on the system's default output (`ClickPlayer`), each
landing louder the further the puck fell; where no output opens, the game plays on in silence.

## Building

The window is built on reckless-toolkit, free software under the GPL, whose files are a git subtree in this
repository at `external/reckless-toolkit` - committed here, so a clone has them and every commit builds on
its own. `tools/toolkit-pull.sh` brings its head in; it is pulled, never edited here. `-DKNECT4_GUI=OFF`
builds the game and its suites alone, without it.

On Windows, in Visual Studio's developer environment (`VsDevCmd.bat -arch=x64 -host_arch=x64 -no_logo`), with CMake and
Ninja:

```
cmake --preset release && cmake --build --preset release && ctest --preset release
build/release/src/app/Knect4.exe
```

VS Code's default build task builds the Debug configuration, its test task runs the suites, and the "Knect 4" launch
entry starts it under the debugger.

### Building on Linux

On Debian, Ubuntu or Mint, with gcc:

```
sudo apt install build-essential cmake ninja-build pkg-config git libfreetype-dev libfontconfig1-dev libx11-dev libasound2-dev
git clone git@github.com:reckless-technology/knect4.git
cd knect4
cmake --preset release
cmake --build --preset release
./build/release/src/app/knect4
```

`ctest --preset release` runs the suites; the window's launch (`app_smoke`, label `window`) needs a display, which
`xvfb-run -a ctest --preset release -L window` gives it where there is none. The `sanitize` preset builds everything
under AddressSanitizer and UndefinedBehaviorSanitizer, the `tsan` preset under ThreadSanitizer (gcc or clang, Linux or
macOS); each has a build and a test preset of the same name. The `sanitize` test preset leaves out the leaks of the
system's PipeWire ALSA plugin, which the audio suite reaches through the toolkit's ALSA back end
(`tools/lsan_suppressions.txt`), and the `tsan` test preset the uninstrumented PulseAudio client libraries the
same back end reaches on a PulseAudio machine (`tools/tsan_suppressions.txt`).

### Building on macOS

With Xcode's command-line tools (`xcode-select --install`) and Homebrew's CMake and Ninja (`brew install cmake ninja`):

```
git clone git@github.com:reckless-technology/knect4.git
cd knect4
cmake --preset release-universal
cmake --build --preset release-universal
open build/release-universal/src/app/Knect4.app
```

The `release-universal` preset builds one binary for Apple silicon and Intel; every binary runs on macOS 13.3 and later.
`ctest --preset release-universal` runs the suites.

### The tools

- `build/release/src/app/knect4_render <out.png> [mid|drop|won|lost|empty|friend|friend-won]` draws the window in that
  state to a PNG, and `knect4_render <out.png> icon <size>` the icon; `docs/images/` holds its pictures. The suites run
  it for every state.
- `powershell -ExecutionPolicy Bypass -File tools\coverage-windows.ps1` measures the line coverage of `src/` (clang-cl
  and LLVM's source-based coverage, the `coverage-windows` preset) over every suite, the window's launch and the render
  tool's runs, and fails under 95 %; `src/app/main.cpp`, which opens the window and the event loop, is left out.
- `python tools/make_icon.py build/release/src/app/knect4_render.exe` draws the icon at every size into
  `assets/icon/` (Pillow); the build takes them from there and needs no Python.
- `python3 tools/third_party_notices.py build/release` writes `THIRD-PARTY-NOTICES.txt` from the build tree's fetches and
  the toolkit's pins (any Python 3, nothing installed); the CTest `notices` runs it with `--check` and fails when the
  committed file is not what it writes. Run it when the toolkit moves, and read the diff.
- C++ is formatted with clang-format 22.1.8 (`.clang-format`), which the pre-commit hook and CI check. In a fresh clone:
  `git config core.hooksPath .githooks`.

### The version

Every binary reports `MAJOR.MINOR.BUILD` (`cmake/knect4_version.cmake`): `MAJOR.MINOR` is the nearest tag of the form
`vMAJOR.MINOR` reachable from the commit built, and `BUILD` the number of commits since it. A checkout with no such tag
builds as 0.0.0, with a warning.

## Packaging

`packaging/` turns a release build tree into what is shipped, the same way on a pull request's run and a release's:

- `packaging/stage.py <build-dir> <dist-dir> windows|linux|macos` stages the game (`Knect4.exe`, `knect4` or
  `Knect4.app`), `LICENSE.txt` and `THIRD-PARTY-NOTICES.txt` into a folder of their own; everything after it reads the
  stage.
- Windows: `packaging/windows/make_installer.ps1` compiles `knect4.iss` with Inno Setup 6.7.3 (fetched and held to its
  hash with `-Fetch`) into `knect4-<version>-win64-setup.exe`. It installs for the current user by default, under the
  user's Programs folder in `Reckless Technology\Knect 4`, or, at the dialog's choice, for every user under
  `Program Files\Reckless Technology\Knect 4`, beside the organisation's other products, with a Start-menu entry in the
  shared `Reckless Technology` group. Its uninstaller takes away what it installed and nothing else.
  `test_installer.ps1` installs a test build of it under a folder of its own, checks every file, launches the installed
  game for its launch check, installs it again over itself and uninstalls it, with two sibling products' folders beside
  it left as they were.
- macOS: `packaging/macos/make_installer.sh` makes `knect4-<version>-macos.pkg`, for macOS 13.3 and later on either kind
  of Mac: `Knect4.app` and `Uninstall Knect 4.app` in `/Applications/Reckless Technology`, the licence and the notices
  in `/Library/Documentation/Knect 4`. The uninstaller asks, then removes those (each bundle only when its identifier is
  the game's) and the package's receipts, removes the shared folder only when it is empty, and removes the person's
  settings only if asked. `test_installer.sh` holds the package to the stage and runs the uninstaller's script on a
  stand-in for `/` beside another product's app. `sign.sh` signs the staged app for a release.
- Linux: `packaging/linux/make_deb.sh` makes the Debian package `knect4-<version>-linux-amd64.deb` - the game at
  `/opt/reckless-technology/bin/knect4`, a menu entry, the icon, the licence, the notices and a copyright file under
  `/usr/share/doc/knect4` - its Depends read off the binary; `test_deb.sh` holds it to the stage.
  `make_tarball.sh` makes the portable `knect4-<version>-linux-x64.tar.xz`, the stage in one folder, and holds it to the
  stage.

## CI

`.github/workflows/build.yml`, on GitHub's hosted runners (`ubuntu-24.04`, `windows-2025`, `macos-15`), on every push to
`main`, every pull request and by hand:

- Windows with MSVC builds everything, runs every suite including the window's launch and the notices' check, makes the
  installer unsigned and puts it through `test_installer.ps1`, and keeps `Knect4.exe` and the installer as the run's
  artifacts `knect4-windows` and `knect4-windows-installer`.
- Linux with gcc builds the game as it ships, runs every suite and the window's launch under `xvfb-run`, makes the
  Debian package and the tarball, and keeps them as `knect4-linux`.
- macOS builds the `release-universal` preset, holds every binary to both slices (`lipo`) and the floor of 13.3
  (`vtool`), runs every suite (and the game's suite again under Rosetta 2), makes the installer package unsigned, and
  keeps it as `knect4-macos`.
- The sanitizer jobs build everything and run every suite under AddressSanitizer and UndefinedBehaviorSanitizer, and
  again under ThreadSanitizer.
- The coverage job builds with gcc instrumented, runs every suite but the window's launch, and fails under 95 % of the
  lines of `src/` (gcovr 8.3; the report kept as `knect4-coverage`); another job checks the formatting.

The artifacts are kept for 30 days. A newer push to a pull request cancels its older run; every commit on `main` keeps
its own. The toolkit needs no checkout of its own and no key: its files are in this repository.

## Releases

`.github/workflows/release.yml` runs on a pushed tag `vMAJOR.MINOR`, or by hand on `main`
(`gh workflow run release.yml --ref main`, with `-f dry_run=true` to make nothing but the run's artifacts). It works out
the version as the build does, refuses a commit `main` does not hold, opens a draft GitHub release `vMAJOR.MINOR.BUILD`,
builds from nothing on the three platforms, runs every suite, packages as above and uploads the four files - the
Windows installer, the macOS package, the Debian package and the tarball - onto the draft, then checks they are all
there. The release stays a private draft: nothing in the workflow publishes it, makes its tag or copies it elsewhere. A
draft of the same version from an earlier run is replaced; a published one stops the run.

Signing uses the organisation's secrets where the repository has them, and is left out with a warning where it has not:
on Windows the installer and its uninstaller are signed through SSL.com's eSigner (the toolkit's
`tools/esigner-sign.ps1`, when `ESIGNER_USERNAME` is set); on macOS the app is signed with the Developer ID Application
identity and the hardened runtime, the package with the Developer ID Installer identity, then notarised and stapled (the
toolkit's `tools/sign-mac.sh`, when `MACOS_DEVELOPER_ID_APPLICATION_P12` is set).

## Licence

Knect 4's own code is under the GNU General Public License version 3 or later (`LICENSE`), copyright 2014-2026
Jonny Reckless, who holds every line of it and so licenses it as he chooses. Its window builds on reckless-toolkit,
which is Jonny's too: the game core - its rules, search and the drop's feel - builds and tests with
`-DKNECT4_GUI=OFF`, which leaves the toolkit out. The third-party parts the
executable contains (Skia with libpng, zlib and, on Linux, FreeType; Noto Serif) are listed with
their licences in `THIRD-PARTY-NOTICES.txt`, which every package installs and the executable carries.

## History

The game began in 2014 in C# with Windows Forms, in the `software` repository under `ConnectFour/`, and was extracted
with its history by git-filter-repo. Its C# sources are in this repository's history up to the commit that replaced them
with the C++ port.
