# CLAUDE.md

Guidance for Claude Code in this repository. The README is the long-form reference: the game, the feel, building, CI.

Knect 4, against the computer or between two people at one board, C++20, its window on reckless-toolkit (a private submodule at
`external/reckless-toolkit`, added with its GUI and its audio device layer on, its suites and ASIO off). Private;
proprietary, Jonny Reckless's.

## Layout

- `src/game/` (`knect4_game`): the board, its evaluation and the negamax search - the 2014 C# `Game.cs` in C++,
  unchanged in behaviour. Equal-score moves are ordered as the .NET Framework 4.8's `List.Sort` ordered them
  (`Game::SortBestFirst`), so the port plays the original's games move for move; `tests/data/csharp_self_play.txt`
  holds it to them. Jonny, 2026-10-21: keep the board representation, search and evaluation as they are for now; they
  will be worked on later - a change to them is a decision of his, and the fixture is then recorded again.
- `src/effects/` (`knect4_effects`): `DropMotion` (the fall and its bounces), `RenderClick` and `ClickMixer` (the
  landing's click, made at the output's rate, mixed on the audio thread without locks). No toolkit.
- `src/app/` (`knect4_gui`, `Knect4`, `knect4_render`): `Controller` (turns, against the computer or between two
  people - Red kMin, Yellow kMax - the search on a thread of its own through `std::async` with a stop flag, the
  computer's paced turn, the drop and its clicks, a score for each kind of game), `BoardView`
  (draws a `BoardPicture` scaled to fit; pucks behind the frame, held puck and banner in front), `board_art` (the
  board's geometry in design pixels and its drawing), `SidePanel`, `MainView`, `menus`, `settings_file` (the person's
  choices in `reckless-technology/knect4/settings.json`, each read defensively), `ClickPlayer` (the toolkit's audio
  layer, default output), `theme.h` (the Reckless dark look: cream ink, amber accent, Noto Serif).
- `tests/`: `knect4_game_test` (game and effects), `tests/app/knect4_app_test` (controller, menus and settings file),
  and the window's launch (`app_smoke`).
- The name is Knect 4 (Jonny, 2026-10-21), chosen over the 2014 original's name, a third party's trademark.

## Rules

- File and directory names are lower_snake_case (Jonny's rule for every repository); classes PascalCase, members
  `snake_case_`, constants `kPascalCase`, booleans `is`/`has`/`does`/`may`, names spelled out; `/** */` on
  declarations with `@param`/`@return`; British spelling in comments and documents.
- clang-format 22.1.8 only (the pre-commit hook and CI check it).
- Every commit authored and committed as `Jonny Reckless <jonny@recklesstechnology.com>`
  (`git config core.hooksPath .githooks` installs the hooks that check it). Work on a branch, open a pull request, merge
  when CI is green.
- CI runs on the organisation's self-hosted runners only (the repository is private).
- The game is a consumer of the toolkit: a bump of the submodule points at a commit on the toolkit's `main` (CI checks).
- Line coverage of `src/` stays at 95 % or more (`tools/coverage-windows.ps1`, CI's coverage job): every new behaviour
  gets a test; the views are driven by events and painted into images in `tests/app/views_test.cpp`, the audio output
  through the toolkit's offline API in `tests/app/click_player_test.cpp`.
