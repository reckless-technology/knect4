#include "app/about_view.h"

#include "app/board_art.h"
#include "app/theme.h"

#include "Embedded.h"

#include <utility>

namespace knect4::app
{

namespace
{

using reckless::gfx::RectF;
using reckless::gfx::RectI;

/**
 * @param name A file the binary carries.
 * @return Its text; empty where the binary carries none.
 */
std::string EmbeddedText(const char *name)
{
    const auto bytes = reckless::embedded::Find(name);
    return std::string(reinterpret_cast<const char *>(bytes.data()), bytes.size());
}

/** @return The scrolling text's look: the window's colours, the interface's face a size small. */
reckless::view::TextView::Style TextStyle()
{
    reckless::view::TextView::Style style;
    style.font       = fonts::Small().WithHeight(14.5f);
    style.text       = theme::kInk;
    style.background = theme::kWindow.Darker(0.3f);
    style.outline    = theme::kRule;
    style.highlight  = theme::kSelect;
    style.padding    = 10;
    return style;
}

} // namespace

AboutView::AboutView(std::string version, std::string licence, std::string notices) : version_(std::move(version))
{
    SetOpaque(false);
    if (licence.empty())
    {
        licence = EmbeddedText("licence.txt");
    }
    if (notices.empty())
    {
        notices = EmbeddedText("third_party_notices.txt");
    }
    text_            = std::make_unique<reckless::view::TextView>(TextStyle());
    std::string text = licence + (notices.empty() || licence.empty() ? std::string() : "\n\n") + notices;
    if (text.empty())
    {
        // A build that carries neither file: where the reader finds them.
        text = "The licence (MIT) and the third-party notices are in LICENSE and THIRD-PARTY-NOTICES.txt, installed "
               "beside the game and in its source.";
    }
    text_->SetText(text);
    AddChild(*text_);
    close_ = std::make_unique<reckless::view::Button>("Close", theme::PlatePainter(), [this] { Close(); });
    AddChild(*close_);
}

const std::string &AboutView::text() const noexcept
{
    return text_->text();
}

RectI AboutView::CardArea() const noexcept
{
    const int card_width  = std::min(kCardWidth, width() - 40);
    const int card_height = std::min(kCardHeight, height() - 40);
    return RectI{(width() - card_width) / 2, (height() - card_height) / 2, card_width, card_height};
}

void AboutView::Paint(reckless::gfx::Canvas &c)
{
    c.FillAll(theme::kWindow.Darker(0.6f).WithAlpha(0.7f));
    const RectF card = CardArea().ToFloat();
    c.SetColor(theme::kPanel);
    c.FillRoundedRect(card, 12.0f);
    c.SetColor(theme::kRule);
    c.DrawRoundedRect(card.Reduced(0.5f), 12.0f, 1.0f);
    // The game's mark: a red puck and a yellow one, overlapping.
    art::DrawPuck(c, {card.x() + 48.0f, card.y() + 52.0f}, 24.0f, theme::kRed);
    art::DrawPuck(c, {card.x() + 78.0f, card.y() + 52.0f}, 24.0f, theme::kYellow);
    c.SetFont(fonts::Headline());
    c.SetColor(theme::kInk);
    c.DrawText("Knect 4", RectF{card.x() + 118.0f, card.y() + 20.0f, card.width() - 140.0f, 36.0f},
               reckless::gfx::Justify::kCentredLeft, true);
    c.SetFont(fonts::Small());
    c.SetColor(theme::kMuted);
    c.DrawText("Version " + version_ + "  ·  © 2014–2026 Jonny Reckless  ·  MIT licence",
               RectF{card.x() + 118.0f, card.y() + 56.0f, card.width() - 140.0f, 22.0f},
               reckless::gfx::Justify::kCentredLeft, true);
}

void AboutView::Resized()
{
    const RectI card = CardArea();
    text_->SetBounds({card.x() + 20, card.y() + 96, card.width() - 40, card.height() - 96 - 70});
    close_->SetBounds({card.right() - 140, card.bottom() - 56, 120, 38});
}

void AboutView::MouseDown(const reckless::view::MouseEvent &e)
{
    if (!CardArea().ToFloat().Contains(e.position))
    {
        Close();
    }
}

bool AboutView::WantsKeyboardFocus() const
{
    return true;
}

bool AboutView::KeyPressed(const reckless::view::KeyEvent &key)
{
    if (key.key == reckless::view::Key::kEscape || key.key == reckless::view::Key::kReturn)
    {
        Close();
        return true;
    }
    return false;
}

void AboutView::Close()
{
    if (on_close)
    {
        const auto closed = on_close;
        closed();
    }
}

} // namespace knect4::app
