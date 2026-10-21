#pragma once
/// @file theme.h The window's colours and fonts: the Reckless apps' dark look (the cream ink, the amber accent, Noto
/// Serif), and the board's blue plastic and the pucks' red and yellow.

#include "AppTheme.h"
#include "Button.h"
#include "Color.h"
#include "LookDrawing.h"
#include "LookPalette.h"
#include "MenuBarView.h"
#include "PanelStyle.h"
#include "Text.h"

#include <string_view>

namespace knect4::app::theme
{

using reckless::gfx::Color;

inline constexpr Color kWindow{0xff29282cu};   ///< The window's colour, behind the board.
inline constexpr Color kPanel{0xff1b1a1du};    ///< The side panel: one step darker.
inline constexpr Color kRow{0xff222125u};      ///< A row of a list.
inline constexpr Color kRule{0xff3b3936u};     ///< The rules and the frames round a section.
inline constexpr Color kInk{0xffe8dec8u};      ///< The text.
inline constexpr Color kMuted{0xff9c9483u};    ///< Secondary text: a caption, a count's label.
inline constexpr Color kFaint{0xff5f5a52u};    ///< A hint, a disabled item.
inline constexpr Color kAmber{0xffe2a23eu};    ///< The accent: a lit plate, the winning line's glow.
inline constexpr Color kAmberDim{0xff6e5428u}; ///< The accent at rest: a plate's edge.
inline constexpr Color kSelect{0xff5d4423u};   ///< The menu under the pointer.
inline constexpr Color kGood{0xff86c9a2u};     ///< A win.
inline constexpr Color kBad{0xffe2705fu};      ///< A loss.
inline constexpr Color kPlate{0xff232125u};    ///< A button's plate.

/** The board's blue plastic, from its lit top to its shaded foot, and the stand's darker blue. */
inline constexpr Color kBoardLight{0xff3a82e0u};
inline constexpr Color kBoardMid{0xff2363c4u};
inline constexpr Color kBoardDark{0xff17479bu};
inline constexpr Color kStand{0xff163f8au};

/** A puck's colours: its body, the light its raised rim catches and the shade beneath it. */
struct PuckColours
{
    Color body;  ///< The face.
    Color light; ///< Where the light falls.
    Color shade; ///< Where it does not.
};

inline constexpr PuckColours kRed{Color{0xffd8312cu}, Color{0xfff4736bu}, Color{0xff7e1210u}};
inline constexpr PuckColours kYellow{Color{0xfff3c419u}, Color{0xffffe680u}, Color{0xffa67a00u}};

/** @return The window's colours as the toolkit's app shell and its widgets take them. */
inline reckless::app::Theme Shell()
{
    return {kWindow, kRule, kPanel, kRow, kInk, kMuted, kFaint, kAmber, kAmberDim, kGood, kBad};
}

/** @return The series' palette for the buttons: the plate a step lighter than the panel, its edge the accent at rest.
 */
inline reckless::look::Palette Buttons()
{
    return {kWindow, kInk, kAmber, kPlate, kAmberDim};
}

} // namespace knect4::app::theme

namespace knect4::app::fonts
{

using reckless::gfx::Font;

/** @return The interface's text. */
inline Font Body()
{
    return Font(19.0f);
}

/** @return Smaller text: a caption, a count's label. */
inline Font Small()
{
    return Font(16.0f);
}

/** @return A section's label, in capitals, bold and spaced. */
inline Font Label()
{
    return Font(15.5f).Bold().WithKerning(0.08f);
}

/** @return A plate's caption: the body face, bold, a little smaller. */
inline Font Plate()
{
    return Font(16.5f).Bold();
}

/** @return The game's state, large: "Your move", "I win". */
inline Font Headline()
{
    return Font(30.0f).Bold();
}

/** @return A count in the score. */
inline Font Count()
{
    return Font(34.0f).Bold();
}

} // namespace knect4::app::fonts

namespace knect4::app::theme
{

/**
 * The window's buttons: the series' button plate in the window's colours (look::DrawButtonPlate), with a toggle's lamp
 * at its left (look::DrawPlateLamp), and its caption in the interface's face.
 * @return The painter.
 */
inline reckless::view::Button::Painter PlatePainter()
{
    return [](reckless::gfx::Canvas &c, reckless::gfx::RectI bounds, std::string_view text,
              reckless::view::Button::Look look) {
        reckless::look::DrawButtonPlate(c, Buttons(), bounds, look.on, look.over, look.held);
        if (look.lamp)
        {
            reckless::look::DrawPlateLamp(c, bounds, look.on);
        }
        c.SetFont(fonts::Plate());
        c.SetColor(look.dimmed ? kInk.WithAlpha(0.4f) : kInk);
        const float left = look.lamp ? 18.0f : 4.0f;
        c.DrawText(text,
                   reckless::gfx::RectF{left, 0.0f, static_cast<float>(bounds.width()) - left - 4.0f,
                                        static_cast<float>(bounds.height())},
                   reckless::gfx::Justify::kCentred, true);
    };
}

/**
 * The plate a choice is drawn on, chosen or not: the window's plate, and over the chosen one the accent - a wash of
 * amber and an amber edge - so a group of choices shows at a glance which is taken.
 * @param c The canvas.
 * @param bounds The plate.
 * @param look How it stands.
 */
inline void DrawChoicePlate(reckless::gfx::Canvas &c, reckless::gfx::RectI bounds, reckless::view::Button::Look look)
{
    reckless::look::DrawButtonPlate(c, Buttons(), bounds, false, look.over, look.held);
    if (look.on)
    {
        const reckless::gfx::RectF plate = bounds.ToFloat().Reduced(1.5f);
        c.SetColor(kAmber.WithAlpha(0.16f));
        c.FillRoundedRect(plate, 4.0f);
        c.SetColor(kAmber);
        c.DrawRoundedRect(plate, 4.0f, 1.5f);
    }
}

/**
 * A choice in a group (a level, who starts): the choice plate with its caption, amber while it is the one taken.
 * @return The painter.
 */
inline reckless::view::Button::Painter ChoicePainter()
{
    return [](reckless::gfx::Canvas &c, reckless::gfx::RectI bounds, std::string_view text,
              reckless::view::Button::Look look) {
        DrawChoicePlate(c, bounds, look);
        c.SetFont(fonts::Plate());
        c.SetColor(look.on ? kAmber.Brighter(0.25f) : kInk);
        c.DrawText(text,
                   reckless::gfx::RectF{4.0f, 0.0f, static_cast<float>(bounds.width()) - 8.0f,
                                        static_cast<float>(bounds.height())},
                   reckless::gfx::Justify::kCentred, true);
    };
}

/**
 * The drawn menu bar (Linux's, and the window drawn to an image): the panel's colour under the interface's face.
 * @return The bar's look.
 */
inline reckless::app::MenuBarStyle MenuBarLook()
{
    reckless::app::MenuBarStyle style;
    style.background    = kPanel;
    style.rule          = kRule;
    style.text          = kInk;
    style.disabled_text = kFaint;
    style.highlight     = kSelect;
    style.font          = fonts::Body().WithHeight(18.5f);
    style.height        = 34;
    style.padding       = 11;
    style.menu          = reckless::panel::MenuStyle(Shell().Palette());
    return style;
}

} // namespace knect4::app::theme
