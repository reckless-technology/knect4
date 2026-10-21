#pragma once
/// @file board_art.h Where everything on the board stands, in the board's design pixels, and how the board, its stand
/// and the pucks are drawn: a frame of blue plastic standing upright on two feet, a round hole for every slot, and
/// pucks with a raised rim and a grooved face.

#include "theme.h"

#include "Canvas.h"
#include "Geometry.h"

#include <vector>

namespace connect4::app::art
{

using reckless::gfx::PointF;
using reckless::gfx::RectF;

inline constexpr int   kColumns      = 7;      ///< The board's columns.
inline constexpr int   kRows         = 6;      ///< Its rows.
inline constexpr float kDesignWidth  = 760.0f; ///< The board and its stand across, in its design pixels.
inline constexpr float kDesignHeight = 704.0f; ///< From above the hovering puck to the shadow under the feet.
inline constexpr float kPitch        = 80.0f;  ///< From one slot's centre to the next.
inline constexpr float kHoleRadius   = 32.0f;  ///< A hole in the frame.
inline constexpr float kPuckRadius   = 30.5f;  ///< A puck's face, a little inside its hole.
inline constexpr float kFrameMargin  = 22.0f;  ///< The frame round the holes.
inline constexpr float kFrameLeft    = 78.0f;  ///< The frame's left edge.
inline constexpr float kFrameTop     = 108.0f; ///< Its top edge.
inline constexpr float kFrameWidth   = kColumns * kPitch + 2.0f * kFrameMargin; ///< The frame across.
inline constexpr float kFrameHeight  = kRows * kPitch + 2.0f * kFrameMargin;    ///< The frame down.
inline constexpr float kHoverY       = 50.0f;                         ///< The centre of the puck held above the board.
inline constexpr RectF kBanner{kFrameLeft, 4.0f, kFrameWidth, 88.0f}; ///< Where the game's end is told, over the board.

/**
 * @param column A column, 0 at the left; a fraction for a puck between two.
 * @return The x of the column's slots' centres.
 */
constexpr float ColumnX(float column) noexcept
{
    return kFrameLeft + kFrameMargin + kPitch / 2.0f + kPitch * column;
}

/**
 * @param row A row, 0 at the bottom.
 * @return The y of the row's slots' centres.
 */
constexpr float RowY(int row) noexcept
{
    return kFrameTop + kFrameMargin + kPitch / 2.0f + kPitch * static_cast<float>(kRows - 1 - row);
}

/**
 * The column under `x`: any x over the frame's width counts, its margins with the columns beside them.
 * @param x An x in the board's design pixels.
 * @return The column, or -1 outside the frame.
 */
int ColumnAt(float x) noexcept;

/**
 * A puck's face: its body shaded as a disc lit from the upper left, a raised rim round it, a recessed face inside with
 * two grooves, and a gloss.
 * @param c The canvas.
 * @param centre Its centre.
 * @param radius Its radius.
 * @param colours Its colours.
 */
void DrawPuck(reckless::gfx::Canvas &c, PointF centre, float radius, const theme::PuckColours &colours);

/**
 * The shadow the board and its feet cast on what they stand on, drawn before anything else.
 * @param c The canvas.
 */
void DrawTableShadow(reckless::gfx::Canvas &c);

/**
 * The frame, in front of the pucks: blue plastic with a hole for every slot, each hole's edge bevelled and its top
 * casting a shadow onto whatever puck stands behind it, and the slots along the top that the pucks drop through.
 * @param c The canvas.
 */
void DrawFrame(reckless::gfx::Canvas &c);

/**
 * The stand: a post down each side of the frame, holding it, on a foot.
 * @param c The canvas.
 */
void DrawStand(reckless::gfx::Canvas &c);

/**
 * A ring round a slot: the winning line's glow, or the faint mark of where a puck would land.
 * @param c The canvas.
 * @param centre The slot's centre.
 * @param colour The ring's colour.
 * @param thickness Its width.
 */
void DrawSlotRing(reckless::gfx::Canvas &c, PointF centre, reckless::gfx::Color colour, float thickness);

} // namespace connect4::app::art
