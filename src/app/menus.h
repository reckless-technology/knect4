#pragma once
/// @file menus.h The menu bar: Game, with a new game, the level, the pucks' colours, who starts, the sound and Quit,
/// each choice ticked as the controller stands; and what each command does to the controller.

#include "controller.h"

#include "MenuBar.h"

#include <functional>

namespace knect4::app
{

/** The menu's commands, as the bar reports them. */
enum Command : int
{
    kNoCommand      = 0,  ///< Nothing.
    kNewGame        = 1,  ///< A new game.
    kEasy           = 2,  ///< Level: Easy.
    kMedium         = 3,  ///< Level: Medium.
    kHard           = 4,  ///< Level: Hard.
    kRedPucks       = 5,  ///< The person plays red.
    kYellowPucks    = 6,  ///< The person plays yellow.
    kPersonStarts   = 7,  ///< The person starts the next game.
    kComputerStarts = 8,  ///< The computer does.
    kSound          = 9,  ///< The sound on or off.
    kQuit           = 10, ///< The app ends.
};

/**
 * The bar, its ticks as `settings` stand.
 * @param settings The person's choices.
 * @return The bar.
 */
reckless::app::MenuBar BuildMenuBar(const Settings &settings);

/**
 * A command carried out on the controller.
 * @param command The command.
 * @param controller The game.
 * @param quit Ends the app, for kQuit; may be empty.
 */
void RunCommand(int command, Controller &controller, const std::function<void()> &quit);

} // namespace knect4::app
