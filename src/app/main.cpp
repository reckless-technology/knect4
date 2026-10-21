// Knect 4: the window on the machine's own platform, through the toolkit's app shell (AppRun.h, AppRoot.h) and
// its child window on Skia, the clicks on the system's output. `--smoke [frames]` is a launch's check: the window
// opened, a few frames drawn, closed, with no sound - exit 0 when every frame was drawn, 3 when too few were, 1 where
// no window could be shown.

#include "app/click_player.h"
#include "app/controller.h"
#include "app/main_view.h"
#include "app/menus.h"
#include "app/settings_file.h"
#include "app/theme.h"
#include "version.h"

#include "AppPlatform.h"
#include "AppRoot.h"
#include "AppRun.h"
#include "Embedded.h"
#include "SkiaCanvas.h"
#include "Window.h"

#include <cstdio>
#include <filesystem>
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
    using knect4::app::MainView;
    const reckless::app::AppOptions options =
        reckless::app::ParseAppOptions(std::vector<std::string>(argv + 1, argv + argc));
    reckless::gfx::SetSerifFaces(reckless::embedded::Find("NotoSerif-Regular.ttf"),
                                 reckless::embedded::Find("NotoSerif-Bold.ttf"));

    const std::unique_ptr<reckless::app::AppPlatform> platform =
        reckless::app::AppPlatform::Create("Knect 4", reckless::embedded::Find("icon_256.png"));

    // The clicks on the system's output; none in a launch's check, which may run where there is no output.
    knect4::app::ClickPlayer clicks;
    if (!options.is_smoke && !clicks.Open())
    {
        std::fprintf(stderr, "Knect 4: no sound (%s)\n", clicks.error().c_str());
    }
    // The person's choices as they left them (settings_file.h); a launch's check reads and keeps none.
    const std::filesystem::path settings_file =
        options.is_smoke ? std::filesystem::path() : knect4::app::DefaultSettingsFile();
    knect4::app::Controller controller(options.is_smoke ? nullptr : &clicks, knect4::app::ReadSettings(settings_file));

    MainView view;
    if (!platform->HasNativeMenuBar())
    {
        view.SetMenuBarView(platform->MakeMenuBarView(knect4::app::theme::MenuBarLook()));
    }
    const auto quit = [&platform] { platform->CallSoon([&platform] { platform->Quit(); }); };
    // Whenever a choice changes: the menu bar set again with its ticks, and the choices kept.
    std::optional<knect4::app::Settings> shown_settings;
    const auto                           settings_changed = [&] {
        const knect4::app::Settings &now = controller.settings();
        if (shown_settings == now)
        {
            return;
        }
        if (shown_settings)
        {
            knect4::app::WriteSettings(settings_file, now);
        }
        shown_settings = now;
        platform->SetMenuBar(knect4::app::BuildMenuBar(now),
                             [&](int command) { knect4::app::RunCommand(command, controller, quit); });
    };

    knect4::app::BoardView &board = view.board();
    board.on_hover                = [&](int column) { controller.Hover(column); };
    board.on_drop                 = [&](int column) { controller.Drop(column); };
    board.on_step                 = [&](int delta) { controller.Step(delta); };
    board.on_drop_held            = [&] { controller.DropHeld(); };
    board.on_play_again           = [&] { controller.NewGame(); };
    knect4::app::SidePanel &panel = view.panel();
    panel.on_opponent             = [&](bool is_two_player) { controller.SetTwoPlayer(is_two_player); };
    panel.on_level                = [&](knect4::app::Level level) { controller.SetLevel(level); };
    panel.on_colour               = [&](bool is_person_red) { controller.SetPersonRed(is_person_red); };
    panel.on_starter              = [&](bool does_person_start) { controller.SetPersonStarts(does_person_start); };
    panel.on_sound                = [&](bool has_sound) { controller.SetSound(has_sound); };
    panel.on_new_game             = [&] { controller.NewGame(); };

    reckless::app::AppRoot root(view, knect4::app::theme::Shell().Tooltip(), {},
                                {MainView::kLayoutWidth, MainView::kLayoutHeight});
    root.SetBounds({MainView::kDesignWidth, MainView::kDesignHeight});

    reckless::app::AppRunHooks hooks;
    hooks.has_menu_bar = true;
    hooks.on_open      = [&](reckless::app::AppWindow &window) {
        static_cast<void>(window);
        settings_changed();
        board.GrabKeyboardFocus();
    };
    hooks.on_frame = [&](double seconds) {
        controller.Frame(seconds);
        clicks.Frame(seconds);
        view.Show(controller);
        settings_changed();
        root.Frame();
    };
    return reckless::app::RunApp(*platform, root, {MainView::kMinimumWidth, MainView::kMinimumHeight}, options,
                                 "Knect 4", knect4::kVersion, &reckless::window::CreateChildWindow, hooks);
}
