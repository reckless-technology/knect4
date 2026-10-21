#pragma once
/// @file board_picture.h What the board shows at one moment: the pucks at rest, the one falling, the one held above
/// the board, the hint, the winning line, the pucks let out at a game's end, and the game's end told - plain data the
/// controller fills and the board view draws.

#include "board_art.h"
#include "theme.h"

#include "game/game.h"

#include <optional>
#include <string>
#include <vector>

namespace knect4::app
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
    bool  is_keyboard{false}; ///< The keys put it there: its column is marked on the frame.
};

/** A puck let out through the slider at a game's end, falling out of the frame's foot. */
struct EmptyingPuck
{
    int   column{0};     ///< Its column.
    float row{0.0f};     ///< Where it stands, in rows: its slot's row less how far it has fallen.
    int   player{kMin};  ///< Whose.
    float opacity{1.0f}; ///< 0 to 1: it fades as it leaves the board.
};

/** The board at one moment. */
struct BoardPicture
{
    art::Layout                layout{}; ///< The board's size and where everything stands.
    std::vector<int>           pieces;   ///< The pucks at rest, column by column from the bottom: kMax, kMin or kEmpty.
    std::optional<FallingPuck> falling;  ///< The puck falling, if one is.
    std::optional<HeldPuck>    held;     ///< The puck above the board, if one is.
    std::vector<EmptyingPuck>  emptying; ///< The pucks falling out through the slider.
    float                      slide{0.0f};     ///< How far the slider is open, 0 to 1.
    int                        hint_column{-1}; ///< The column the hint suggests; -1 for none.
    std::vector<Cell>          winning;         ///< The winning line's slots, lit.
    float                      glow{0.0f};      ///< The winning line's glow now, 0 to 1.
    std::string                banner;          ///< The game's end, told over the board; empty while it goes on.
    reckless::gfx::Color       banner_colour{theme::kInk}; ///< The banner's colour.
    theme::PuckColours         computer{theme::kYellow};   ///< kMax's pucks: the computer's, or Yellow's.
    theme::PuckColours         person{theme::kRed};        ///< kMin's: the person's, or Red's.
    bool                       has_marks{false};           ///< The pucks carry their side's mark.
    bool                       may_choose{false};          ///< A person may choose a column: the pointer shows a hand.

    /**
     * @param column A column on the board.
     * @param row A row on the board.
     * @return The puck at rest there: kMax, kMin or kEmpty.
     */
    int At(int column, int row) const noexcept
    {
        const std::size_t index = static_cast<std::size_t>(column * layout.rows() + row);
        return index < pieces.size() ? pieces[index] : kEmpty;
    }
};

} // namespace knect4::app
