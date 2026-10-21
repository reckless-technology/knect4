#pragma once
/// @file board_art.h Where everything on the board stands, in the board's design pixels, for a board of any size, and
/// how the board, its stand and the pucks are drawn: a frame of blue plastic standing upright on two feet, a round hole
/// for every slot, pucks with a raised rim and a grooved face, and the slider under the frame that lets them out.

#include "theme.h"

#include "Canvas.h"
#include "Geometry.h"

namespace knect4::app::art
{

using reckless::gfx::PointF;
using reckless::gfx::RectF;

inline constexpr float kPitch       = 80.0f;  ///< From one slot's centre to the next.
inline constexpr float kHoleRadius  = 32.0f;  ///< A hole in the frame.
inline constexpr float kPuckRadius  = 30.5f;  ///< A puck's face, a little inside its hole.
inline constexpr float kFrameMargin = 22.0f;  ///< The frame round the holes, the slider's rail in its foot.
inline constexpr float kFrameLeft   = 78.0f;  ///< The frame's left edge: room for the stand's post and foot.
inline constexpr float kFrameTop    = 108.0f; ///< Its top edge: room above for the held puck and the banner.
inline constexpr float kHoverY      = 50.0f;  ///< The centre of the puck held above the board.

/**
 * A board's layout, in its design pixels: the frame round `columns` x `rows` holes a pitch apart, the stand either side
 * of it, the held puck and the banner above. The board view scales the whole to fit its view, so a larger board draws
 * its pucks smaller.
 */
class Layout
{
  public:
    /**
     * @param columns The board's columns.
     * @param rows Its rows.
     */
    constexpr Layout(int columns = 7, int rows = 6) noexcept : columns_(columns), rows_(rows)
    {
    }

    /** @return The columns. */
    constexpr int columns() const noexcept
    {
        return columns_;
    }

    /** @return The rows. */
    constexpr int rows() const noexcept
    {
        return rows_;
    }

    /** @return The frame across. */
    constexpr float FrameWidth() const noexcept
    {
        return static_cast<float>(columns_) * kPitch + 2.0f * kFrameMargin;
    }

    /** @return The frame down. */
    constexpr float FrameHeight() const noexcept
    {
        return static_cast<float>(rows_) * kPitch + 2.0f * kFrameMargin;
    }

    /** @return The frame's bottom edge. */
    constexpr float FrameBottom() const noexcept
    {
        return kFrameTop + FrameHeight();
    }

    /** @return The frame's outline. */
    constexpr RectF Frame() const noexcept
    {
        return RectF{kFrameLeft, kFrameTop, FrameWidth(), FrameHeight()};
    }

    /** @return The board and its stand across, in its design pixels. */
    constexpr float DesignWidth() const noexcept
    {
        return FrameWidth() + 2.0f * kFrameLeft;
    }

    /** @return From above the held puck to the shadow under the feet. */
    constexpr float DesignHeight() const noexcept
    {
        return FrameBottom() + 72.0f;
    }

    /** @return Where the game's end is told, over the board. */
    constexpr RectF Banner() const noexcept
    {
        return RectF{kFrameLeft, 4.0f, FrameWidth(), 88.0f};
    }

    /**
     * @param column A column, 0 at the left; a fraction for a puck between two.
     * @return The x of the column's slots' centres.
     */
    constexpr float ColumnX(float column) const noexcept
    {
        return kFrameLeft + kFrameMargin + kPitch / 2.0f + kPitch * column;
    }

    /**
     * @param row A row, 0 at the bottom; a fraction, or below 0, for a puck on its way.
     * @return The y of the row's slots' centres.
     */
    constexpr float RowY(float row) const noexcept
    {
        return kFrameTop + kFrameMargin + kPitch / 2.0f + kPitch * (static_cast<float>(rows_ - 1) - row);
    }

    /**
     * The column under `x`: any x over the frame's width counts, its margins with the columns beside them.
     * @param x An x in the board's design pixels.
     * @return The column, or -1 outside the frame.
     */
    int ColumnAt(float x) const noexcept;

    /**
     * @param row The row a puck lands in.
     * @return How far it falls from where it is held, in rows.
     */
    constexpr double FallTo(int row) const noexcept
    {
        return static_cast<double>((RowY(static_cast<float>(row)) - kHoverY) / kPitch);
    }

    /** @param other Another layout. @return The two the same. */
    constexpr bool operator==(const Layout &other) const noexcept = default;

  private:
    int columns_; ///< The columns.
    int rows_;    ///< The rows.
};

/**
 * A puck's face: its body shaded as a disc lit from the upper left, a raised rim round it, a recessed face inside with
 * two grooves, and a gloss; with its side's mark on it where `has_mark` is set.
 * @param c The canvas.
 * @param centre Its centre.
 * @param radius Its radius.
 * @param colours Its colours and mark.
 * @param has_mark Whether the mark is drawn.
 */
void DrawPuck(reckless::gfx::Canvas &c, PointF centre, float radius, const theme::PuckColours &colours,
              bool has_mark = false);

/**
 * The shadow the board and its feet cast on what they stand on, drawn before anything else.
 * @param c The canvas.
 * @param layout The board's layout.
 */
void DrawTableShadow(reckless::gfx::Canvas &c, const Layout &layout);

/**
 * The frame, in front of the pucks: blue plastic with a hole for every slot, each hole's edge bevelled and its top
 * casting a shadow onto whatever puck stands behind it, the slots along the top that the pucks drop through, and the
 * slider in its foot that holds them in, drawn `slide` of the way open.
 * @param c The canvas.
 * @param layout The board's layout.
 * @param slide How far the slider is drawn open, 0 shut to 1 open.
 */
void DrawFrame(reckless::gfx::Canvas &c, const Layout &layout, float slide = 0.0f);

/**
 * The stand: a post down each side of the frame, holding it, on a foot.
 * @param c The canvas.
 * @param layout The board's layout.
 */
void DrawStand(reckless::gfx::Canvas &c, const Layout &layout);

/**
 * A ring round a slot: the winning line's glow, the hint, or the faint mark of where a puck would land.
 * @param c The canvas.
 * @param centre The slot's centre.
 * @param colour The ring's colour.
 * @param thickness Its width.
 */
void DrawSlotRing(reckless::gfx::Canvas &c, PointF centre, reckless::gfx::Color colour, float thickness);

/**
 * The keyboard's marker: an amber chevron on the frame's top edge over the column the keys hold the puck over.
 * @param c The canvas.
 * @param layout The board's layout.
 * @param column The column.
 */
void DrawKeyboardMarker(reckless::gfx::Canvas &c, const Layout &layout, int column);

} // namespace knect4::app::art
