#include "app/board_view.h"

#include "app/board_art.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace knect4::app
{

namespace
{

using reckless::gfx::PointF;
using reckless::gfx::RectF;

/**
 * @param picture The picture.
 * @param player kMax or kMin.
 * @return That player's colours.
 */
const theme::PuckColours &ColoursOf(const BoardPicture &picture, int player) noexcept
{
    return player == kMax ? picture.computer : picture.person;
}

/** The empty space round the board in its view, in the board's design pixels. */
constexpr float kRoom = 12.0f;

} // namespace

BoardView::BoardView()
{
    SetOpaque(false);
}

void BoardView::SetPicture(BoardPicture picture)
{
    picture_ = std::move(picture);
    cursor   = picture_.may_choose && column_under_pointer_ >= 0 ? reckless::view::Cursor::kPointingHand
                                                                 : reckless::view::Cursor::kArrow;
    if (!picture_.banner.empty() && is_play_again_lit_)
    {
        cursor = reckless::view::Cursor::kPointingHand;
    }
    Repaint();
}

RectF BoardView::PlayAgainArea(const art::Layout &layout) noexcept
{
    const RectF banner = layout.Banner();
    return RectF{banner.right() - 172.0f, banner.CentreY() - 22.0f, 156.0f, 44.0f};
}

reckless::gfx::Affine BoardView::BoardTransform() const noexcept
{
    const float        available_width  = std::max(static_cast<float>(width()), 1.0f);
    const float        available_height = std::max(static_cast<float>(height()), 1.0f);
    const art::Layout &layout           = picture_.layout;
    const float        scale            = std::min(available_width / (layout.DesignWidth() + 2.0f * kRoom),
                                                   available_height / (layout.DesignHeight() + 2.0f * kRoom));
    const float        left             = (available_width - layout.DesignWidth() * scale) / 2.0f;
    const float        top              = (available_height - layout.DesignHeight() * scale) / 2.0f;
    return reckless::gfx::Affine::Scale(scale).Translated(left, top);
}

PointF BoardView::ToBoard(PointF point) const noexcept
{
    const reckless::gfx::Affine transform = BoardTransform();
    const PointF                origin    = transform.Apply({0.0f, 0.0f});
    const PointF                unit      = transform.Apply({1.0f, 1.0f});
    const float                 scale     = std::max(unit.x - origin.x, 1e-6f);
    return {(point.x - origin.x) / scale, (point.y - origin.y) / scale};
}

void BoardView::Paint(reckless::gfx::Canvas &c)
{
    const reckless::gfx::ScopedState state(c);
    c.AddTransform(BoardTransform());
    PaintBoard(c, picture_, is_play_again_lit_);
}

void BoardView::PaintBoard(reckless::gfx::Canvas &c, const BoardPicture &picture, bool is_play_again_lit)
{
    const art::Layout &layout = picture.layout;
    const auto         slot   = [&layout](int column, float row) {
        return PointF{layout.ColumnX(static_cast<float>(column)), layout.RowY(row)};
    };
    art::DrawTableShadow(c, layout);

    // Behind the frame: the pucks at rest, the one falling, and those let out through the slider.
    for (int column = 0; column != layout.columns(); ++column)
    {
        for (int row = 0; row != layout.rows(); ++row)
        {
            if (const int piece = picture.At(column, row); piece != kEmpty)
            {
                art::DrawPuck(c, slot(column, static_cast<float>(row)), art::kPuckRadius, ColoursOf(picture, piece),
                              picture.has_marks);
            }
        }
    }
    if (picture.falling)
    {
        const FallingPuck &falling = *picture.falling;
        const float        y       = art::kHoverY + static_cast<float>(falling.drop) * art::kPitch;
        art::DrawPuck(c, {layout.ColumnX(static_cast<float>(falling.column)), y}, art::kPuckRadius,
                      ColoursOf(picture, falling.player), picture.has_marks);
    }
    for (const EmptyingPuck &puck : picture.emptying)
    {
        c.BeginTransparencyLayer(std::clamp(puck.opacity, 0.0f, 1.0f));
        art::DrawPuck(c, slot(puck.column, puck.row), art::kPuckRadius, ColoursOf(picture, puck.player),
                      picture.has_marks);
        c.EndTransparencyLayer();
    }

    art::DrawFrame(c, layout, picture.slide);
    art::DrawStand(c, layout);

    // In front: the winning line's glow, the hint, where the held puck would land, and the held puck.
    if (!picture.winning.empty())
    {
        const float strength = 0.45f + 0.55f * std::clamp(picture.glow, 0.0f, 1.0f);
        for (const Cell &cell : picture.winning)
        {
            const PointF centre = slot(cell.column, static_cast<float>(cell.row));
            art::DrawSlotRing(c, centre, theme::kAmber.WithAlpha(0.35f * strength), 9.0f);
            art::DrawSlotRing(c, centre, theme::kAmber.WithAlpha(strength), 3.5f);
        }
    }
    const auto landing_row = [&picture, &layout](int column) {
        int row = 0;
        while (row < layout.rows() && picture.At(column, row) != kEmpty)
        {
            ++row;
        }
        return row;
    };
    if (picture.hint_column >= 0 && picture.hint_column < layout.columns())
    {
        if (const int row = landing_row(picture.hint_column); row < layout.rows())
        {
            const PointF centre = slot(picture.hint_column, static_cast<float>(row));
            art::DrawSlotRing(c, centre, theme::kAmber.WithAlpha(0.3f), 10.0f);
            art::DrawSlotRing(c, centre, theme::kAmber, 3.0f);
        }
    }
    if (picture.held)
    {
        const HeldPuck &held    = *picture.held;
        const float     opacity = std::clamp(held.opacity, 0.0f, 1.0f);
        if (held.landing_column >= 0 && held.landing_column < layout.columns())
        {
            if (const int row = landing_row(held.landing_column); row < layout.rows())
            {
                art::DrawSlotRing(c, slot(held.landing_column, static_cast<float>(row)),
                                  ColoursOf(picture, held.player).light.WithAlpha(0.55f * opacity), 3.0f);
            }
            if (held.is_keyboard)
            {
                art::DrawKeyboardMarker(c, layout, held.landing_column);
            }
        }
        if (opacity > 0.0f)
        {
            c.BeginTransparencyLayer(opacity);
            art::DrawPuck(c, {layout.ColumnX(held.column), art::kHoverY}, art::kPuckRadius,
                          ColoursOf(picture, held.player), picture.has_marks);
            c.EndTransparencyLayer();
        }
    }

    // The game's end, over the board where the held puck hung.
    if (!picture.banner.empty())
    {
        const RectF banner = layout.Banner();
        c.SetColor(theme::kPanel.WithAlpha(0.94f));
        c.FillRoundedRect(banner, 12.0f);
        c.SetColor(theme::kRule);
        c.DrawRoundedRect(banner.Reduced(0.5f), 12.0f, 1.0f);
        c.SetFont(fonts::Headline());
        c.SetColor(picture.banner_colour);
        c.DrawText(picture.banner, RectF{banner.x() + 26.0f, banner.y(), banner.width() - 220.0f, banner.height()},
                   reckless::gfx::Justify::kCentredLeft, true);
        // The plate painted as a button paints it, at its own origin.
        const RectF                      area = PlayAgainArea(layout);
        const reckless::gfx::ScopedState plate_state(c);
        c.AddTransform(reckless::gfx::Affine::Translation(area.x(), area.y()));
        theme::PlatePainter()(
            c, reckless::gfx::RectI{0, 0, static_cast<int>(area.width()), static_cast<int>(area.height())},
            "Play again", reckless::view::Button::Look{false, is_play_again_lit, false, false, false});
    }
}

void BoardView::Track(PointF point)
{
    const PointF board  = ToBoard(point);
    const bool   is_lit = !picture_.banner.empty() && PlayAgainArea(picture_.layout).Contains(board);
    if (is_lit != is_play_again_lit_)
    {
        is_play_again_lit_ = is_lit;
        Repaint();
    }
    const bool is_over_board = board.y >= 0.0f && board.y <= picture_.layout.FrameBottom();
    const int  column        = is_over_board ? picture_.layout.ColumnAt(board.x) : -1;
    // Every move over the board is told, not only a change of column: the keys may have moved the held puck since, and
    // the pointer moving takes it back to where the pointer is, so a click drops where the puck is seen.
    if (column >= 0 || column != column_under_pointer_)
    {
        column_under_pointer_ = column;
        if (on_hover)
        {
            on_hover(column);
        }
    }
    cursor = (picture_.may_choose && column >= 0) || is_lit ? reckless::view::Cursor::kPointingHand
                                                            : reckless::view::Cursor::kArrow;
}

void BoardView::MouseMove(const reckless::view::MouseEvent &e)
{
    Track(e.position);
}

void BoardView::MouseDrag(const reckless::view::MouseEvent &e)
{
    Track(e.position);
}

void BoardView::MouseExit(const reckless::view::MouseEvent &e)
{
    static_cast<void>(e);
    is_play_again_lit_    = false;
    column_under_pointer_ = -1;
    if (on_hover)
    {
        on_hover(-1);
    }
    Repaint();
}

void BoardView::MouseDown(const reckless::view::MouseEvent &e)
{
    GrabKeyboardFocus();
    if (e.right)
    {
        return;
    }
    Track(e.position);
    if (is_play_again_lit_)
    {
        if (on_play_again)
        {
            on_play_again();
        }
        return;
    }
    if (column_under_pointer_ >= 0 && on_drop)
    {
        on_drop(column_under_pointer_);
    }
}

bool BoardView::WantsKeyboardFocus() const
{
    return true;
}

bool BoardView::KeepsKeyboardFocus() const
{
    return true;
}

bool BoardView::KeyPressed(const reckless::view::KeyEvent &key)
{
    using reckless::view::Key;
    if (key.modifiers.command || key.modifiers.alt)
    {
        return false;
    }
    switch (key.key)
    {
    case Key::kLeft:
    case Key::kRight:
        if (on_step)
        {
            on_step(key.key == Key::kLeft ? -1 : 1);
        }
        return true;
    case Key::kReturn:
    case Key::kDown:
        if (!picture_.banner.empty() && key.key == Key::kReturn)
        {
            if (on_play_again)
            {
                on_play_again();
            }
        }
        else if (on_drop_held)
        {
            on_drop_held();
        }
        return true;
    case Key::kCharacter:
        // 1 to 9 drop into the first nine columns; a wider board's others are reached with the arrows.
        if (key.character >= U'1' && key.character <= U'9' &&
            key.character < U'1' + static_cast<char32_t>(picture_.layout.columns()))
        {
            if (on_drop)
            {
                on_drop(static_cast<int>(key.character - U'1'));
            }
            return true;
        }
        if (key.character == U' ')
        {
            if (on_drop_held)
            {
                on_drop_held();
            }
            return true;
        }
        return false;
    default:
        return false;
    }
}

} // namespace knect4::app
