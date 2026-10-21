#pragma once
/// @file about_view.h Help > About Knect 4: a card over the window with the game's name, version and copyright, its
/// licence (MIT) and the notices of the third-party parts the program carries, read from the files it embeds.

#include "Button.h"
#include "TextView.h"
#include "View.h"

#include <functional>
#include <memory>
#include <string>

namespace knect4::app
{

/**
 * The About card over a dimmed window: the name, the version, the copyright, and the licence and the third-party
 * notices in a scrolling text. A click outside the card, Escape, or "Close" closes it (`on_close`).
 */
class AboutView final : public reckless::view::View
{
  public:
    static constexpr int kCardWidth  = 620; ///< The card across, in design pixels.
    static constexpr int kCardHeight = 560; ///< Down.

    /**
     * @param version The program's version.
     * @param licence The licence's text; the embedded `licence.txt` where empty.
     * @param notices The third-party notices; the embedded `third_party_notices.txt` where empty.
     */
    AboutView(std::string version, std::string licence = {}, std::string notices = {});

    std::function<void()> on_close; ///< The card closed.

    /** @return The text shown in the scrolling view. */
    const std::string &text() const noexcept;

    /** @return The card's place in the view. */
    reckless::gfx::RectI CardArea() const noexcept;

    void Paint(reckless::gfx::Canvas &c) override;
    void Resized() override;
    void MouseDown(const reckless::view::MouseEvent &e) override;
    bool WantsKeyboardFocus() const override;
    bool KeyPressed(const reckless::view::KeyEvent &key) override;

  private:
    /** The card closed, told. */
    void Close();

    std::string                               version_; ///< The version.
    std::unique_ptr<reckless::view::TextView> text_;    ///< The licence and the notices.
    std::unique_ptr<reckless::view::Button>   close_;   ///< "Close".
};

} // namespace knect4::app
