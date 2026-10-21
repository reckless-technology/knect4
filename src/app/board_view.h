#pragma once
/// @file board_view.h The board: drawn from a BoardPicture at the largest size its view holds, telling what the person
/// does over it - the column the pointer is over, a click to drop, the keys, "Play again".

#include "board_picture.h"

#include "Canvas.h"
#include "Geometry.h"
#include "View.h"

#include <functional>

namespace connect4::app
{

/**
 * The board and its stand, scaled to fit the view and centred in it. The pucks at rest and the falling one are drawn
 * behind the frame, so a falling puck shows through the holes as it passes them; the held puck and the banner in front.
 */
class BoardView final : public reckless::view::View
{
  public:
    BoardView();

    std::function<void(int column)> on_hover;      ///< The pointer is over a column, or -1 off the board.
    std::function<void(int column)> on_drop;       ///< A click on a column, or the key of its number (1 to 7).
    std::function<void(int delta)>  on_step;       ///< The left or right arrow: the held puck a column over.
    std::function<void()>           on_drop_held;  ///< Return, space or the down arrow: the held puck dropped.
    std::function<void()>           on_play_again; ///< The banner's "Play again".

    /**
     * What the board shows from now on.
     * @param picture The picture.
     */
    void SetPicture(BoardPicture picture);

    /** @return What it shows. */
    const BoardPicture &picture() const noexcept
    {
        return picture_;
    }

    /**
     * The board at its design size, as the view would draw it, onto any canvas: the render tool's and the icon's.
     * @param c The canvas, in the board's design pixels.
     * @param picture What to show.
     * @param is_play_again_lit Whether the banner's button is drawn under the pointer.
     */
    static void PaintBoard(reckless::gfx::Canvas &c, const BoardPicture &picture, bool is_play_again_lit = false);

    /** @return Where the banner's "Play again" stands, in the board's design pixels. */
    static reckless::gfx::RectF PlayAgainArea() noexcept;

    /**
     * @param point A point in the view.
     * @return The same point in the board's design pixels.
     */
    reckless::gfx::PointF ToBoard(reckless::gfx::PointF point) const noexcept;

    void Paint(reckless::gfx::Canvas &c) override;
    void MouseMove(const reckless::view::MouseEvent &e) override;
    void MouseDrag(const reckless::view::MouseEvent &e) override;
    void MouseExit(const reckless::view::MouseEvent &e) override;
    void MouseDown(const reckless::view::MouseEvent &e) override;
    bool WantsKeyboardFocus() const override;
    bool KeyPressed(const reckless::view::KeyEvent &key) override;

  private:
    /** @return The scale the board is drawn at and where its top left lands, in the view. */
    reckless::gfx::Affine BoardTransform() const noexcept;

    /**
     * The pointer at `point`: the column told, the pointer's shape and the banner's button lit.
     * @param point The pointer, in the view.
     */
    void Track(reckless::gfx::PointF point);

    BoardPicture picture_;                  ///< What it shows.
    bool         is_play_again_lit_{false}; ///< The pointer is over "Play again".
    int          column_under_pointer_{-1}; ///< The column last told.
};

} // namespace connect4::app
