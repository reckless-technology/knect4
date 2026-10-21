// Connect Four: the window on the machine's own platform, through the toolkit's app shell (AppRun.h, AppRoot.h) and
// its child window on Skia, the clicks on the system's output. `--smoke [frames]` is a launch's check: the window
// opened, a few frames drawn, closed, with no sound - exit 0 when every frame was drawn, 3 when too few were, 1 where
// no window could be shown.

#include "app/click_player.h"
#include "app/controller.h"
#include "app/main_view.h"
#include "app/menus.h"
#include "app/theme.h"
#include "version.h"

#include "AppPlatform.h"
#include "AppRoot.h"
#include "AppRun.h"
#include "Embedded.h"
#include "SkiaCanvas.h"
#include "Window.h"

#include <cstdio>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

/**
 * The game's window.
 * @param argc The argument count.
 * @param argv The arguments.
 * @return The exit code (reckless::app::RunApp).
 */
int main(int argc, char **argv)
{
    using connect4::app::MainView;
    const reckless::app::AppOptions options =
        reckless::app::ParseAppOptions(std::vector<std::string>(argv + 1, argv + argc));
    reckless::gfx::SetSerifFaces(reckless::embedded::Find("NotoSerif-Regular.ttf"),
                                 reckless::embedded::Find("NotoSerif-Bold.ttf"));

    const std::unique_ptr<reckless::app::AppPlatform> platform =
        reckless::app::AppPlatform::Create("Connect Four", reckless::embedded::Find("icon_256.png"));

    // The clicks on the system's output; none in a launch's check, which may run where there is no output.
    connect4::app::ClickPlayer clicks;
    if (!options.is_smoke && !clicks.Open())
    {
        std::fprintf(stderr, "Connect Four: no sound (%s)\n", clicks.error().c_str());
    }
    connect4::app::Controller controller(options.is_smoke ? nullptr : &clicks);

    MainView view;
    if (!platform->HasNativeMenuBar())
    {
        view.SetMenuBarView(platform->MakeMenuBarView(connect4::app::theme::MenuBarLook()));
    }
    const auto quit = [&platform] { platform->CallSoon([&platform] { platform->Quit(); }); };
    // The menu bar, set again whenever a tick on it would change.
    std::optional<connect4::app::Settings> menu_settings;
    const auto                             refresh_menu = [&] {
        const connect4::app::Settings &now = controller.settings();
        if (menu_settings && menu_settings->level == now.level && menu_settings->is_person_red == now.is_person_red &&
            menu_settings->does_person_start == now.does_person_start && menu_settings->has_sound == now.has_sound)
        {
            return;
        }
        menu_settings = now;
        platform->SetMenuBar(connect4::app::BuildMenuBar(now),
                             [&](int command) { connect4::app::RunCommand(command, controller, quit); });
    };

    connect4::app::BoardView &board = view.board();
    board.on_hover                  = [&](int column) { controller.Hover(column); };
    board.on_drop                   = [&](int column) { controller.Drop(column); };
    board.on_step                   = [&](int delta) { controller.Step(delta); };
    board.on_drop_held              = [&] { controller.DropHeld(); };
    board.on_play_again             = [&] { controller.NewGame(); };
    connect4::app::SidePanel &panel = view.panel();
    panel.on_level                  = [&](connect4::app::Level level) { controller.SetLevel(level); };
    panel.on_colour                 = [&](bool is_person_red) { controller.SetPersonRed(is_person_red); };
    panel.on_starter                = [&](bool does_person_start) { controller.SetPersonStarts(does_person_start); };
    panel.on_sound                  = [&](bool has_sound) { controller.SetSound(has_sound); };
    panel.on_new_game               = [&] { controller.NewGame(); };

    reckless::app::AppRoot root(view, connect4::app::theme::Shell().Tooltip(), {},
                                {MainView::kLayoutWidth, MainView::kLayoutHeight});
    root.SetBounds({MainView::kDesignWidth, MainView::kDesignHeight});

    reckless::app::AppRunHooks hooks;
    hooks.has_menu_bar = true;
    hooks.on_open      = [&](reckless::app::AppWindow &window) {
        static_cast<void>(window);
        refresh_menu();
        board.GrabKeyboardFocus();
    };
    hooks.on_frame = [&](double seconds) {
        controller.Frame(seconds);
        clicks.Frame(seconds);
        view.Show(controller);
        refresh_menu();
        root.Frame();
    };
    return reckless::app::RunApp(*platform, root, {MainView::kMinimumWidth, MainView::kMinimumHeight}, options,
                                 "Connect Four", connect4::kVersion, &reckless::window::CreateChildWindow, hooks);
}
