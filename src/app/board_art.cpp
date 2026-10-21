#include "app/board_art.h"

#include "Path.h"

#include <algorithm>
#include <cmath>

namespace connect4::app::art
{

namespace
{

using reckless::gfx::Color;
using reckless::gfx::Gradient;
using reckless::gfx::Path;

constexpr Color kBlack{0xff000000u}; ///< Shadow, at an alpha.
constexpr Color kWhite{0xffffffffu}; ///< Light, at an alpha.

/** @return The frame's outline: its rounded rectangle. */
RectF FrameRect() noexcept
{
    return RectF{kFrameLeft, kFrameTop, kFrameWidth, kFrameHeight};
}

/**
 * @param centre A circle's centre.
 * @param radius Its radius.
 * @return The square it fills.
 */
RectF Circle(PointF centre, float radius) noexcept
{
    return RectF{centre.x - radius, centre.y - radius, 2.0f * radius, 2.0f * radius};
}

/**
 * A linear blend from `from` to `to`, with a stop between.
 * @param from Where it starts.
 * @param from_colour Its colour there.
 * @param to Where it ends.
 * @param to_colour Its colour there.
 * @param stops The stops between, as proportions of the way.
 * @return The gradient.
 */
Gradient Linear(PointF from, Color from_colour, PointF to, Color to_colour,
                std::vector<std::pair<float, Color>> stops = {})
{
    return Gradient{from_colour, from, to_colour, to, false, std::move(stops)};
}

/**
 * A radial blend from `centre` out to `radius`.
 * @param centre Its centre.
 * @param centre_colour Its colour there.
 * @param radius How far it reaches.
 * @param rim_colour Its colour there and beyond.
 * @param stops The stops between.
 * @return The gradient.
 */
Gradient Radial(PointF centre, Color centre_colour, float radius, Color rim_colour,
                std::vector<std::pair<float, Color>> stops = {})
{
    return Gradient{centre_colour, centre, rim_colour, PointF{centre.x + radius, centre.y}, true, std::move(stops)};
}

/**
 * A ring lit as an edge is: `lit` toward the upper left, `unlit` toward the lower right.
 * @param c The canvas.
 * @param centre Its centre.
 * @param radius The ring's middle.
 * @param thickness Its width.
 * @param lit Its colour where the light falls.
 * @param unlit Its colour opposite.
 */
void LitRing(reckless::gfx::Canvas &c, PointF centre, float radius, float thickness, Color lit, Color unlit)
{
    c.SetGradient(Linear({centre.x - radius * 0.7f, centre.y - radius * 0.7f}, lit,
                         {centre.x + radius * 0.7f, centre.y + radius * 0.7f}, unlit));
    c.DrawEllipse(Circle(centre, radius), thickness);
}

} // namespace

int ColumnAt(float x) noexcept
{
    if (x < kFrameLeft || x >= kFrameLeft + kFrameWidth)
    {
        return -1;
    }
    const int column = static_cast<int>(std::floor((x - kFrameLeft - kFrameMargin) / kPitch));
    return std::clamp(column, 0, kColumns - 1);
}

void DrawPuck(reckless::gfx::Canvas &c, PointF centre, float radius, const theme::PuckColours &colours)
{
    const float r = radius;
    // The body: a disc lit from the upper left.
    c.SetGradient(Radial({centre.x - 0.35f * r, centre.y - 0.4f * r}, colours.light, 1.35f * r, colours.shade,
                         {{0.45f, colours.body}}));
    c.FillEllipse(Circle(centre, r));
    // The raised rim catches the light at the upper left and falls into shade at the lower right.
    LitRing(c, centre, 0.88f * r, 0.17f * r, colours.light, colours.shade);
    // The face inside it is set down a step: its edge in shade where the rim's is lit.
    c.SetColor(colours.body.Darker(0.06f));
    c.FillEllipse(Circle(centre, 0.785f * r));
    LitRing(c, centre, 0.775f * r, 0.05f * r, colours.shade.WithAlpha(0.8f), colours.light.WithAlpha(0.7f));
    // Two grooves pressed into the face, each a dark line with the light along its lower lip.
    for (const float groove : {0.56f, 0.34f})
    {
        c.SetColor(colours.shade.WithAlpha(0.55f));
        c.DrawEllipse(Circle(centre, groove * r), 1.4f);
        c.SetColor(colours.light.WithAlpha(0.35f));
        c.DrawEllipse(Circle({centre.x + 0.4f, centre.y + 0.9f}, groove * r), 0.9f);
    }
    // A gloss on the plastic, and the edge.
    const PointF gloss{centre.x - 0.38f * r, centre.y - 0.45f * r};
    c.SetGradient(Radial(gloss, kWhite.WithAlpha(0.45f), 0.5f * r, kWhite.WithAlpha(0.0f)));
    c.FillEllipse(Circle(gloss, 0.5f * r));
    c.SetColor(colours.shade.WithAlpha(0.7f));
    c.DrawEllipse(Circle(centre, r - 0.5f), 1.0f);
}

void DrawTableShadow(reckless::gfx::Canvas &c)
{
    const reckless::gfx::ScopedState state(c);
    const PointF                     centre{kDesignWidth / 2.0f, 682.0f};
    // A circle's soft shadow squashed flat: an ellipse as wide as the stand.
    c.AddTransform(reckless::gfx::Affine::Scale(1.0f, 0.045f, centre.x, centre.y));
    c.SetGradient(Radial(centre, kBlack.WithAlpha(0.5f), kDesignWidth / 2.0f, kBlack.WithAlpha(0.0f),
                         {{0.6f, kBlack.WithAlpha(0.3f)}}));
    c.FillEllipse(Circle(centre, kDesignWidth / 2.0f));
}

void DrawFrame(reckless::gfx::Canvas &c)
{
    const RectF frame = FrameRect();
    Path        plastic;
    plastic.AddRoundedRect(frame, 16.0f);
    for (int column = 0; column != kColumns; ++column)
    {
        for (int row = 0; row != kRows; ++row)
        {
            plastic.AddEllipse(Circle({ColumnX(static_cast<float>(column)), RowY(row)}, kHoleRadius));
        }
    }
    plastic.SetFillRule(Path::FillRule::kEvenOdd);

    // The plastic, lit from above.
    c.SetGradient(Linear({0.0f, frame.y()}, theme::kBoardLight, {0.0f, frame.bottom()}, theme::kBoardDark,
                         {{0.4f, theme::kBoardMid}}));
    c.FillPath(plastic);
    {
        // A sheen across its upper part.
        const reckless::gfx::ScopedState state(c);
        c.ClipToPath(plastic);
        c.SetGradient(
            Linear({0.0f, frame.y()}, kWhite.WithAlpha(0.22f), {0.0f, frame.y() + 90.0f}, kWhite.WithAlpha(0.0f)));
        c.FillRect(RectF{frame.x(), frame.y(), frame.width(), 90.0f});
    }

    // Each hole: the frame's thickness casting a shadow down into it, onto the puck behind, and its edge bevelled.
    for (int column = 0; column != kColumns; ++column)
    {
        for (int row = 0; row != kRows; ++row)
        {
            const PointF centre{ColumnX(static_cast<float>(column)), RowY(row)};
            {
                const reckless::gfx::ScopedState state(c);
                Path                             hole;
                hole.AddEllipse(Circle(centre, kHoleRadius));
                c.ClipToPath(hole);
                Path shadow;
                shadow.AddEllipse(Circle(centre, kHoleRadius));
                shadow.AddEllipse(Circle({centre.x + 2.5f, centre.y + 8.0f}, kHoleRadius));
                shadow.SetFillRule(Path::FillRule::kEvenOdd);
                c.SetColor(kBlack.WithAlpha(0.34f));
                c.FillPath(shadow);
            }
            c.SetGradient(Linear({centre.x, centre.y - kHoleRadius}, kBlack.WithAlpha(0.6f),
                                 {centre.x, centre.y + kHoleRadius}, kWhite.WithAlpha(0.45f),
                                 {{0.5f, theme::kBoardMid.WithAlpha(0.6f)}}));
            c.DrawEllipse(Circle(centre, kHoleRadius - 1.6f), 3.6f);
        }
    }

    // The slots along the top the pucks drop through.
    for (int column = 0; column != kColumns; ++column)
    {
        const float x = ColumnX(static_cast<float>(column));
        c.SetColor(kBlack.WithAlpha(0.6f));
        c.FillRoundedRect(RectF{x - 30.0f, frame.y() + 6.0f, 60.0f, 7.0f}, 3.5f);
        c.SetColor(kWhite.WithAlpha(0.18f));
        c.DrawLine({x - 28.0f, frame.y() + 14.0f}, {x + 28.0f, frame.y() + 14.0f}, 1.0f);
    }

    // The frame's edge: lit along the top, shaded along the foot.
    c.SetGradient(Linear({0.0f, frame.y()}, kWhite.WithAlpha(0.35f), {0.0f, frame.bottom()}, kBlack.WithAlpha(0.3f)));
    c.DrawRoundedRect(frame.Reduced(1.5f), 14.5f, 2.0f);
    c.SetColor(theme::kBoardDark.Darker(0.35f));
    c.DrawRoundedRect(frame, 16.0f, 1.2f);
}

void DrawStand(reckless::gfx::Canvas &c)
{
    const float top    = kFrameTop - 14.0f;
    const float bottom = 668.0f;
    for (const float post_left : {kFrameLeft - 22.0f, kFrameLeft + kFrameWidth - 4.0f})
    {
        // The post: rounded, lit across its middle.
        const RectF post{post_left, top, 26.0f, bottom - top};
        c.SetGradient(Linear({post.x(), 0.0f}, theme::kStand.Darker(0.2f), {post.right(), 0.0f},
                             theme::kStand.Darker(0.35f), {{0.35f, theme::kBoardMid}}));
        c.FillRoundedRect(post, 9.0f);
        c.SetColor(kWhite.WithAlpha(0.15f));
        c.DrawRoundedRect(post.Reduced(1.0f), 8.0f, 1.0f);
        // The clips that hold the frame, top and bottom.
        for (const float y : {kFrameTop + 26.0f, kFrameTop + kFrameHeight - 26.0f})
        {
            const PointF clip{post.CentreX(), y};
            c.SetGradient(Radial({clip.x - 2.0f, clip.y - 2.0f}, theme::kBoardLight, 8.0f, theme::kStand.Darker(0.2f)));
            c.FillEllipse(Circle(clip, 7.0f));
            c.SetColor(kBlack.WithAlpha(0.35f));
            c.DrawEllipse(Circle(clip, 7.0f), 1.0f);
        }
        // The foot: a wide wedge the post stands in.
        const float middle = post.CentreX();
        Path        foot;
        foot.MoveTo(middle - 30.0f, 646.0f);
        foot.LineTo(middle + 30.0f, 646.0f);
        foot.QuadTo(middle + 36.0f, 646.0f, middle + 40.0f, 652.0f);
        foot.LineTo(middle + 60.0f, 676.0f);
        foot.QuadTo(middle + 62.0f, 682.0f, middle + 55.0f, 682.0f);
        foot.LineTo(middle - 55.0f, 682.0f);
        foot.QuadTo(middle - 62.0f, 682.0f, middle - 60.0f, 676.0f);
        foot.LineTo(middle - 40.0f, 652.0f);
        foot.QuadTo(middle - 36.0f, 646.0f, middle - 30.0f, 646.0f);
        foot.Close();
        c.SetGradient(Linear({0.0f, 646.0f}, theme::kBoardMid, {0.0f, 682.0f}, theme::kStand.Darker(0.3f)));
        c.FillPath(foot);
        c.SetColor(kWhite.WithAlpha(0.2f));
        c.DrawLine({middle - 29.0f, 647.0f}, {middle + 29.0f, 647.0f}, 1.0f);
    }
}

void DrawSlotRing(reckless::gfx::Canvas &c, PointF centre, Color colour, float thickness)
{
    c.SetColor(colour);
    c.DrawEllipse(Circle(centre, kHoleRadius - thickness / 2.0f), thickness);
}

} // namespace connect4::app::art
