#pragma once
/// @file menus.h The menu bar - Game, Board, View and Help - each choice ticked and enabled as the game stands, and
/// what each command does.

#include "controller.h"

#include "AppWindowSizes.h"
#include "MenuBar.h"

#include <functional>

namespace knect4::app
{

using reckless::app::kWindowSizes;   ///< The View menu's sizes, the toolkit's (AppWindowSizes.h).
using reckless::app::WindowSizeMenu; ///< What the View menu shows of them.
using reckless::app::WindowSizes;    ///< The window held at them.

/** The menu's commands, as the bar reports them. */
enum Command : int
{
    kNoCommand       = 0,   ///< Nothing.
    kNewGame         = 1,   ///< A new game.
    kEasy            = 2,   ///< Level: Easy.
    kMedium          = 3,   ///< Level: Medium.
    kHard            = 4,   ///< Level: Hard.
    kRedPucks        = 5,   ///< The person plays red.
    kYellowPucks     = 6,   ///< The person plays yellow.
    kPersonStarts    = 7,   ///< The person (between two people, Red) starts the next game.
    kComputerStarts  = 8,   ///< The computer (Yellow) does.
    kSound           = 9,   ///< The sound on or off.
    kQuit            = 10,  ///< The app ends.
    kAgainstComputer = 11,  ///< Play against the computer.
    kAgainstFriend   = 12,  ///< Two people play each other.
    kPerfect         = 13,  ///< Level: Perfect.
    kUndo            = 14,  ///< Take a move back.
    kHint            = 15,  ///< Ask for a hint.
    kAlternate       = 16,  ///< Between two people, the first move alternates.
    kMarks           = 17,  ///< The pucks' marks on or off.
    kAbout           = 18,  ///< The About window.
    kLarger          = 19,  ///< The window a size larger.
    kSmaller         = 20,  ///< A size smaller.
    kBoardBase       = 100, ///< kBoardBase + n: the board size kBoardSizes[n].
    kWindowSizeBase  = 200, ///< kWindowSizeBase + percent: the window at that size.
};

/** What the bar shows: the choices and what can be done now. */
struct MenuState
{
    Settings       settings;        ///< The choices, ticked.
    bool           can_undo{false}; ///< Undo is enabled.
    bool           can_hint{false}; ///< Hint is enabled.
    WindowSizeMenu window;          ///< The window's sizes.

    /** @param other Another state. @return The two show the same bar. */
    bool operator==(const MenuState &other) const = default;
};

/**
 * @param controller The game.
 * @param window The window's sizes as the View menu shows them.
 * @return The bar's state now.
 */
MenuState MenuStateOf(const Controller &controller, const WindowSizeMenu &window);

/**
 * The bar for `state`.
 * @param state What it shows.
 * @return The bar.
 */
reckless::app::MenuBar BuildMenuBar(const MenuState &state);

/** What the commands that reach past the game do. */
struct CommandHandlers
{
    std::function<void()>            quit;        ///< Ends the app; may be empty.
    std::function<void()>            about;       ///< Shows the About window; may be empty.
    std::function<void(int percent)> window_size; ///< Sizes the window; may be empty.
};

/**
 * A command carried out.
 * @param command The command.
 * @param controller The game.
 * @param handlers What the commands past the game do.
 * @param window The window's sizes, for kLarger and kSmaller.
 */
void RunCommand(int command, Controller &controller, const CommandHandlers &handlers = {},
                const WindowSizeMenu &window = {});

} // namespace knect4::app
