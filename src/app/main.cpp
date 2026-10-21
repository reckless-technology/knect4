// Knect 4: the window on the machine's own platform, through the toolkit's app shell (AppRun.h, AppRoot.h) and
// its child window on Skia, the clicks on the system's output. `--smoke [frames]` is a launch's check: the window
// opened, a few frames drawn, closed, with no sound and nothing kept - exit 0 when every frame was drawn, 3 when too
// few were, 1 where no window could be shown.

#include "app/about_view.h"
#include "app/click_player.h"
#include "app/controller.h"
#include "app/main_view.h"
#include "app/menus.h"
#include "app/settings_file.h"
#include "app/theme.h"
#include "app/window_sizes.h"
#include "version.h"

#include "AppPlatform.h"
#include "AppRoot.h"
#include "AppRun.h"
#include "Embedded.h"
#include "SkiaCanvas.h"
#include "Window.h"

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

    // The clicks on the system's output, opened as the first puck falls (click_player.h); none in a launch's check,
    // which may run where there is no output.
    knect4::app::ClickPlayer clicks;
    // The person's choices and the scores as they left them (settings_file.h); a launch's check reads and keeps none.
    knect4::app::SettingsKeeper keeper(options.is_smoke ? std::filesystem::path() : knect4::app::DefaultSettingsFile());
    const knect4::app::Saved    saved = keeper.Read();
    knect4::app::Controller     controller(options.is_smoke ? nullptr : &clicks, saved.settings, saved.scores);

    MainView view;
    if (!platform->HasNativeMenuBar())
    {
        view.SetMenuBarView(platform->MakeMenuBarView(knect4::app::theme::MenuBarLook()));
    }
    reckless::app::AppRoot   root(view, knect4::app::theme::Shell().Tooltip(), {},
                                  {MainView::kLayoutWidth, MainView::kLayoutHeight});
    knect4::app::WindowSizes sizes(root, {MainView::kDesignWidth, MainView::kDesignHeight});
    sizes.Restore(saved.settings.window_percent);

    // Help > About: a card over the window, made when asked for and gone when closed.
    std::unique_ptr<knect4::app::AboutView> about;
    const auto                              show_about = [&] {
        about           = std::make_unique<knect4::app::AboutView>(knect4::kVersion);
        about->on_close = [&] {
            view.SetOverlay(nullptr);
            platform->CallSoon([&] { about.reset(); });
            view.board().GrabKeyboardFocus();
        };
        view.SetOverlay(about.get());
        about->GrabKeyboardFocus();
    };
    knect4::app::CommandHandlers handlers;
    handlers.quit        = [&platform] { platform->CallSoon([&platform] { platform->Quit(); }); };
    handlers.about       = show_about;
    handlers.window_size = [&](int percent) {
        if (sizes.Choose(percent))
        {
            controller.SetWindowPercent(percent);
        }
    };

    // Whenever the choices, the scores or what the menu can do change: kept, and the menu bar set again.
    std::optional<knect4::app::MenuState> menu_state;
    const auto                            follow = [&] {
        keeper.Changed({controller.settings(), controller.scores()});
        const knect4::app::MenuState now = knect4::app::MenuStateOf(controller, sizes.Menu());
        if (menu_state != now)
        {
            menu_state = now;
            platform->SetMenuBar(knect4::app::BuildMenuBar(now), [&](int command) {
                knect4::app::RunCommand(command, controller, handlers, sizes.Menu());
            });
        }
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
    panel.on_alternate            = [&](bool does_alternate) { controller.SetAlternate(does_alternate); };
    panel.on_board                = [&](knect4::BoardSize size) { controller.SetBoardSize(size); };
    panel.on_sound                = [&](bool has_sound) { controller.SetSound(has_sound); };
    panel.on_marks                = [&](bool has_marks) { controller.SetMarks(has_marks); };
    panel.on_new_game             = [&] { controller.NewGame(); };
    panel.on_undo                 = [&] { controller.Undo(); };
    panel.on_hint                 = [&] { controller.AskHint(); };
    panel.on_names                = [&](std::string red, std::string yellow) {
        controller.SetNames(std::move(red), std::move(yellow));
    };

    reckless::app::AppRunHooks hooks;
    hooks.has_menu_bar = true;
    hooks.on_open      = [&](reckless::app::AppWindow &window) {
        sizes.SetWindow(&window);
        follow();
        board.GrabKeyboardFocus();
    };
    hooks.on_frame = [&](double seconds) {
        sizes.Frame(seconds);
        controller.Frame(seconds);
        clicks.Frame(seconds);
        view.Show(controller);
        follow();
        root.Frame();
    };
    hooks.on_end = [&](std::optional<std::pair<int, int>> position) {
        static_cast<void>(position);
        // The size in force kept, or none where the window was dragged to another.
        controller.SetWindowPercent(sizes.InForce());
        keeper.Changed({controller.settings(), controller.scores()});
        sizes.SetWindow(nullptr);
    };
    return reckless::app::RunApp(*platform, root, {MainView::kMinimumWidth, MainView::kMinimumHeight}, options,
                                 "Knect 4", knect4::kVersion, &reckless::window::CreateChildWindow, hooks);
}
