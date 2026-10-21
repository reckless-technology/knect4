#pragma once
/// @file board_picture.h What the board shows at one moment: the pucks at rest, the one falling, the one held above
/// the board, the winning line and the game's end - plain data the controller fills and the board view draws.

#include "theme.h"

#include "game/game.h"

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace connect4::app
{

/** A puck falling down a column, behind the frame. */
struct FallingPuck
{
    int    column{0};    ///< Its column.
    double drop{0.0};    ///< How far below the held puck's place it stands, in rows (DropMotion::position).
    int    player{kMin}; ///< Whose.
};

/** A puck held above the board, ready to drop. */
struct HeldPuck
{
    float column{3.0f};       ///< Where it hangs: a column, or a fraction while it glides between two.
    int   player{kMin};       ///< Whose.
    int   landing_column{-1}; ///< The column whose landing slot is marked; -1 for none.
    float opacity{1.0f};      ///< 0 to 1: it fades in as a turn begins.
};

/** The board at one moment. */
struct BoardPicture
{
    /** The pucks at rest, [column][row], row 0 at the bottom: kMax, kMin or kEmpty. */
    std::array<std::array<int, Game::kDefaultHeight>, Game::kDefaultWidth> pieces{};
    std::optional<FallingPuck>                                             falling; ///< The puck falling, if one is.
    std::optional<HeldPuck> held;       ///< The puck above the board, if one is.
    std::vector<Cell>       winning;    ///< The winning line's slots, lit.
    float                   glow{0.0f}; ///< The winning line's glow now, 0 to 1.
    std::string             banner;     ///< The game's end, told over the board; empty while it goes on.
    reckless::gfx::Color    banner_colour{theme::kInk}; ///< The banner's colour.
    theme::PuckColours      computer{theme::kYellow};   ///< The computer's pucks (kMax).
    theme::PuckColours      person{theme::kRed};        ///< The person's (kMin).
    bool                    may_choose{false};          ///< The person may choose a column: the pointer shows a hand.
};

} // namespace connect4::app
