#pragma once
/// @file window_sizes.h The window's sizes the View menu offers - 50 % to 150 % of the layout's design size, the
/// content drawn at exactly that scale - which one is in force, which the window's screen takes, and the next up and
/// down. Reckless Amphitheater's (amphitheater-chess src/app/WindowSizes.h) with the size kept in the game's settings
/// rather than a file of its own.

#include "AppPlatform.h"
#include "AppRoot.h"
#include "Geometry.h"

#include <array>
#include <cstddef>

namespace knect4::app
{

/** The sizes the View menu offers, in percent of the layout's design size. */
inline constexpr std::array<int, 5> kWindowSizes{50, 75, 100, 125, 150};

/** What the View menu shows of the window's sizes. */
struct WindowSizeMenu
{
    bool                                  has_window{false}; ///< A window to size: none in a suite or the render tool.
    int                                   in_force{0};       ///< The size the window is at, 0 when it was dragged off.
    std::array<bool, kWindowSizes.size()> fits{};            ///< Each size the window's screen takes.
    int                                   larger{0};         ///< The next size up that fits; 0 at the top.
    int                                   smaller{0};        ///< The next size down; 0 at the bottom.

    /**
     * @param a One state.
     * @param b The other.
     * @return Whether every field is the same.
     */
    friend bool operator==(const WindowSizeMenu &a, const WindowSizeMenu &b) = default;
};

/**
 * The window's sizes: the root held at one of kWindowSizes - the content laid out at its design size and drawn at
 * exactly that scale - the window sized to it on its screen, and what the screen has room for asked again now and then.
 */
class WindowSizes
{
  public:
    static constexpr double kRefreshSeconds = 0.5; ///< How often the screen's room is asked again.

    /**
     * @param root The window's root; outlives this.
     * @param design The layout's design size, which 100 % is, in design pixels.
     */
    WindowSizes(reckless::app::AppRoot &root, reckless::gfx::PointI design);

    /**
     * Before the window opens: the root held at `percent`, the window to open round it (RunApp holds it to what its
     * screen takes, which lets the size go where the screen is smaller).
     * @param percent One of kWindowSizes; another is 100.
     */
    void Restore(int percent);

    /** @param window The window to size, once it is open; null as it closes. */
    void SetWindow(reckless::app::AppWindow *window);

    /**
     * A size chosen: the root held at it, the window sized to it and kept on its screen.
     * @param percent One of kWindowSizes.
     * @return Whether it was taken: not for a size not offered, with no window, or one the screen does not take.
     */
    bool Choose(int percent);

    /** @param seconds The time since the last frame: the screen's room asked again every kRefreshSeconds. */
    void Frame(double seconds);

    /** @return What the View menu shows. */
    WindowSizeMenu Menu() const;

    /** @return The size in force, or 0 where the window was dragged to another. */
    int InForce() const;

    /**
     * @param percent A size.
     * @return Whether the View menu offers it.
     */
    static bool IsOffered(int percent);

  private:
    /**
     * @param percent A size.
     * @return The content's size at it, in pixels.
     */
    reckless::gfx::PointI SizeAt(int percent) const;

    /**
     * @param percent A size.
     * @return Whether the window's screen takes it.
     */
    bool Fits(int percent) const;

    /** The screen's room asked again. */
    void RefreshRoom();

    reckless::app::AppRoot   &root_;            ///< The window's root.
    reckless::gfx::PointI     design_;          ///< The design size.
    reckless::app::AppWindow *window_{nullptr}; ///< The window, while it is open.
    reckless::gfx::PointI     room_{0, 0};      ///< The most the window's screen takes; 0 x 0 for unknown.
    double                    elapsed_{0.0};    ///< Seconds since the room was asked.
};

} // namespace knect4::app
