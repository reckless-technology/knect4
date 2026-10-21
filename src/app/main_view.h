#pragma once
/// @file main_view.h The window's content: the board on the left, the side panel on the right, and on Linux the
/// drawn menu bar across the top.

#include "board_view.h"
#include "side_panel.h"

#include "MenuBarView.h"
#include "View.h"

#include <memory>

namespace knect4::app
{

/** The window's views, laid out at whatever size the window has. */
class MainView final : public reckless::view::View
{
  public:
    static constexpr int kDesignWidth   = 1100; ///< The size the window opens at, in design pixels.
    static constexpr int kDesignHeight  = 760;  ///< Its height.
    static constexpr int kLayoutWidth   = 860;  ///< The least the layout takes; smaller is zoomed down.
    static constexpr int kLayoutHeight  = 700;  ///< Its height.
    static constexpr int kMinimumWidth  = 640;  ///< The least the window may be resized to.
    static constexpr int kMinimumHeight = 480;  ///< Its height.

    MainView();

    /** @return The board. */
    BoardView &board() noexcept
    {
        return board_;
    }

    /** @return The side panel. */
    SidePanel &panel() noexcept
    {
        return panel_;
    }

    /**
     * The drawn menu bar across the top, where the platform shows none of its own (Linux).
     * @param bar The bar; null for none.
     */
    void SetMenuBarView(std::unique_ptr<reckless::app::MenuBarView> bar);

    /**
     * The views set from the controller: the board's picture and the panel.
     * @param controller The game.
     */
    void Show(const Controller &controller);

    void Paint(reckless::gfx::Canvas &c) override;
    void Resized() override;

  private:
    std::unique_ptr<reckless::app::MenuBarView> menu_bar_; ///< Linux's drawn bar, or null.
    BoardView                                   board_;    ///< The board.
    SidePanel                                   panel_;    ///< The panel.
};

} // namespace knect4::app
