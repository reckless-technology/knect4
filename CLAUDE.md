# CLAUDE.md

Guidance for Claude Code in this repository. The README is the long-form reference: the game, the feel, building, CI.

Connect Four against the computer, C++20, its window on reckless-toolkit (a private submodule at
`external/reckless-toolkit`, added with its GUI and its audio device layer on, its suites and ASIO off). Private;
proprietary, Jonny Reckless's.

## Layout

- `src/game/` (`connect4_game`): the board, its evaluation and the negamax search - the 2014 C# `Game.cs` in C++,
  unchanged in behaviour. Equal-score moves are ordered as the .NET Framework 4.8's `List.Sort` ordered them
  (`Game::SortBestFirst`), so the port plays the original's games move for move; `tests/data/csharp_self_play.txt`
  holds it to them. Jonny, 2026-10-21: keep the board representation, search and evaluation as they are for now; they
  will be worked on later - a change to them is a decision of his, and the fixture is then recorded again.
- `src/effects/` (`connect4_effects`): `DropMotion` (the fall and its bounces), `RenderClick` and `ClickMixer` (the
  landing's click, made at the output's rate, mixed on the audio thread without locks). No toolkit.
- `src/app/` (`connect4_gui`, `Connect4`, `connect4_render`): `Controller` (turns, the search on a thread of its own
  through `std::async` with a stop flag, the computer's paced turn, the drop and its clicks, the score), `BoardView`
  (draws a `BoardPicture` scaled to fit; pucks behind the frame, held puck and banner in front), `board_art` (the
  board's geometry in design pixels and its drawing), `SidePanel`, `MainView`, `menus`, `ClickPlayer` (the toolkit's
  audio layer, default output), `theme.h` (the Reckless dark look: cream ink, amber accent, Noto Serif).
- `tests/`: `connect4_game_test` (game and effects), `tests/app/connect4_app_test` (controller and menus), and the
  window's launch (`app_smoke`).

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
