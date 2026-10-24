# CLAUDE.md

Guidance for Claude Code in this repository. The README is the long-form reference: the game, the feel, building, CI.

Knect 4, against the computer or between two people at one board, C++20, its window on reckless-toolkit (a private submodule at
`external/reckless-toolkit`, added with its GUI and its audio device layer on, its suites and ASIO off). A private
repository; the game's own code is under the GNU General Public License version 3 or later (`LICENSE`), Jonny
Reckless's, so
outsiders can build the game core with `-DKNECT4_GUI=OFF` but not the window.

## Layout

- `src/game/` (`knect4_game`): `Game` (the board the window shows, and the rules), `BasicPosition` (bitboards in a
  `std::uint64_t` or `WideBits` of two or three words, for the five board sizes in `kBoardSizes`), `Search` (Pons's
  exact solver, depth-limited searches for Easy, Medium and Hard, wins scored by how soon, ties broken at random),
  `Engine` (the sizes behind one interface), `OpeningBook` (Perfect's opening on 7 x 6, `opening_book_data.inc`,
  written by `tools/make_book.cpp` - regenerate it, never edit it, after any change to the solver or the position's
  key). Jonny, 2026-10-21: Easy looks only one or two plies ahead and is easy to beat; Perfect never loses a game it
  could save on the standard board.
- `src/effects/` (`knect4_effects`): `DropMotion` (the fall and its bounces), `RenderClick` and `ClickMixer` (the
  landing's click, made at the output's rate, mixed on the audio thread without locks). No toolkit.
- `src/app/` (`knect4_gui`, `Knect4`, `knect4_render`): `Controller` (turns, against the computer or between two
  people - Red kMin, Yellow kMax - the search on a thread of its own through `std::async` with a stop flag, the
  computer's paced turn, the drop and its clicks, a score for each kind of game), `BoardView`
  (draws a `BoardPicture` scaled to fit; pucks behind the frame, held puck and banner in front), `board_art` (the
  board's geometry for any size, and its drawing), `SidePanel`, `MainView`, `AboutView`, `menus` (the View menu's
  window sizes the toolkit's `WindowSizes`, `AppWindowSizes.h`, the size kept in the settings), `settings_file` (the
  person's choices in `reckless-technology/knect4/settings.json`, each read defensively), `ClickPlayer` (the toolkit's
  audio layer, default output), `theme.h` (the Reckless dark look: cream ink, amber accent, Noto Serif).
- `tests/`: `knect4_game_test` (the board, the engine on every size - the solver held to exhaustive search - and the
  effects), `tests/app/knect4_app_test` (controller, menus, settings file, views painted and driven, the click player
  on the toolkit's offline audio API, window sizes), the render tool's runs and the window's launch (`app_smoke`).
- `THIRD-PARTY-NOTICES.txt`: written by `tools/third_party_notices.py <build-dir>` from the build's fetches and the
  toolkit's pins (CTest `notices` checks it); the executable carries it and `LICENSE` as the embedded files
  `third_party_notices.txt` and `licence.txt`. Write it again when the submodule moves.
- `packaging/`: `stage.py`, then the Windows installer (Inno Setup), the macOS package with its uninstaller and the
  Linux Debian package and tarball, each with a test script; `build.yml` makes them all unsigned on every pull request,
  `release.yml` (a pushed `vMAJOR.MINOR`, or by hand with `dry_run`) builds them from nothing, signs where the secrets
  are set, and leaves a draft release - never published, never promoted from here.
- The version is `MAJOR.MINOR.BUILD` from the nearest `vMAJOR.MINOR` tag (`cmake/knect4_version.cmake`); 0.0.0 with no
  tag reachable.
- The name is Knect 4 (Jonny, 2026-10-21), chosen over the 2014 original's name, a third party's trademark.

## Rules

- File and directory names are lower_snake_case (Jonny's rule for every repository); classes PascalCase, members
  `snake_case_`, constants `kPascalCase`, booleans `is`/`has`/`does`/`may`, names spelled out; `/** */` on
  declarations with `@param`/`@return`; British spelling in comments and documents.
- clang-format 22.1.8 only (the pre-commit hook and CI check it).
- Every commit authored and committed as `Jonny Reckless <jonny@recklesstechnology.com>`
  (`git config core.hooksPath .githooks` installs the hooks that check it). Work on a branch, open a pull request, merge
  when CI is green.
- CI runs on GitHub's hosted runners (the repository is public; `ubuntu-24.04`, `windows-2025`, `macos-15`): Windows,
  Linux (release with the window under `xvfb-run`, coverage, AddressSanitizer with UndefinedBehaviorSanitizer,
  ThreadSanitizer) and macOS universal.
- The game is a consumer of the toolkit: a bump of the submodule points at a commit on the toolkit's `main` (CI checks).
- Line coverage of `src/` stays at 95 % or more (CI's coverage job, gcc and gcovr on Linux; `tools/coverage-windows.ps1`
  on a Windows desk with clang-cl, which the Windows runners lack): every new behaviour gets a test; the views are
  driven by events and painted into images in `tests/app/views_test.cpp`, the audio output through the toolkit's
  offline API in `tests/app/click_player_test.cpp`.
