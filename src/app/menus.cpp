#include "app/menus.h"

#include <string>
#include <utility>
#include <vector>

namespace knect4::app
{

namespace
{

using reckless::app::MenuEntry;
using reckless::app::Shortcut;
using reckless::app::ShortcutKey;

/**
 * An item.
 * @param title Its text.
 * @param command Its command.
 * @param is_checked Whether it shows a tick.
 * @param shortcut Its key; none for no key.
 * @param is_enabled Whether it can be chosen.
 * @return The item.
 */
MenuEntry Item(std::string title, int command, bool is_checked = false, Shortcut shortcut = {}, bool is_enabled = true)
{
    MenuEntry entry = MenuEntry::Item(std::move(title), command, shortcut);
    entry.checked   = is_checked;
    entry.enabled   = is_enabled;
    return entry;
}

/**
 * @param size A board's size.
 * @return Its name in the menu: "7 × 6".
 */
std::string SizeTitle(BoardSize size)
{
    return std::to_string(size.columns) + " × " + std::to_string(size.rows);
}

} // namespace

MenuState MenuStateOf(const Controller &controller, const WindowSizeMenu &window)
{
    return MenuState{controller.settings(), controller.CanUndo(), controller.CanHint(), window};
}

reckless::app::MenuBar BuildMenuBar(const MenuState &state)
{
    // Between two people the level and the colour choice have nothing to set, and who starts is named by colour.
    const Settings &settings = state.settings;
    const bool      is_two   = settings.is_two_player;

    std::vector<MenuEntry> game{
        Item("New game", kNewGame, false, Shortcut::Primary('n')),
        Item("Undo", kUndo, false, Shortcut::Primary('z'), state.can_undo),
        Item("Hint", kHint, false, Shortcut::Primary('h'), state.can_hint),
        MenuEntry::Separator(),
        Item("Against the computer", kAgainstComputer, !is_two),
        Item("Against a friend", kAgainstFriend, is_two, Shortcut::Primary('2', true)),
        MenuEntry::Separator(),
        Item("Easy", kEasy, settings.level == Level::kEasy, Shortcut::Primary('1'), !is_two),
        Item("Medium", kMedium, settings.level == Level::kMedium, Shortcut::Primary('2'), !is_two),
        Item("Hard", kHard, settings.level == Level::kHard, Shortcut::Primary('3'), !is_two),
        Item("Perfect", kPerfect, settings.level == Level::kPerfect, Shortcut::Primary('4'), !is_two),
        MenuEntry::Separator(),
        Item("Red pucks", kRedPucks, settings.is_person_red, {}, !is_two),
        Item("Yellow pucks", kYellowPucks, !settings.is_person_red, {}, !is_two),
        MenuEntry::Separator(),
        Item(is_two ? "Red starts" : "You start", kPersonStarts, settings.does_person_start),
        Item(is_two ? "Yellow starts" : "Computer starts", kComputerStarts, !settings.does_person_start),
        Item("Take turns to start", kAlternate, settings.does_alternate, {}, is_two),
        MenuEntry::Separator(),
        Item("Sound", kSound, settings.has_sound, Shortcut::Primary('m')),
        MenuEntry::Separator(),
        Item("Quit", kQuit, false, Shortcut::Primary('q'))};

    std::vector<MenuEntry> board;
    for (std::size_t index = 0; index < kBoardSizes.size(); ++index)
    {
        board.push_back(Item(SizeTitle(kBoardSizes[index]), kBoardBase + static_cast<int>(index),
                             settings.board == kBoardSizes[index]));
    }

    std::vector<MenuEntry> view{Item("Marked pucks", kMarks, settings.has_marks), MenuEntry::Separator()};
    for (std::size_t index = 0; index < kWindowSizes.size(); ++index)
    {
        const int percent = kWindowSizes[index];
        view.push_back(Item(std::to_string(percent) + " %", kWindowSizeBase + percent, state.window.in_force == percent,
                            {}, state.window.fits[index]));
    }
    MenuEntry larger       = Item("Larger", kLarger, false, Shortcut::Primary('='), state.window.larger != 0);
    larger.other_shortcuts = {Shortcut::Primary('=', true), Shortcut::Primary('+')};
    view.push_back(MenuEntry::Separator());
    view.push_back(std::move(larger));
    view.push_back(Item("Smaller", kSmaller, false, Shortcut::Primary('-'), state.window.smaller != 0));

    reckless::app::MenuBar bar{{MenuEntry::Menu("Game", std::move(game)), MenuEntry::Menu("Board", std::move(board)),
                                MenuEntry::Menu("View", std::move(view)),
                                MenuEntry::Menu("Help", {Item("About Knect 4", kAbout)})}};
    bar.about_id = kAbout; // the Mac app menu's About opens the game's own window
    return bar;
}

void RunCommand(int command, Controller &controller, const CommandHandlers &handlers, const WindowSizeMenu &window)
{
    const auto size_window = [&handlers](int percent) {
        if (percent != 0 && handlers.window_size)
        {
            handlers.window_size(percent);
        }
    };
    if (command >= kBoardBase && command < kBoardBase + static_cast<int>(kBoardSizes.size()))
    {
        controller.SetBoardSize(kBoardSizes[static_cast<std::size_t>(command - kBoardBase)]);
        return;
    }
    if (command > kWindowSizeBase && WindowSizes::IsOffered(command - kWindowSizeBase))
    {
        size_window(command - kWindowSizeBase);
        return;
    }
    switch (command)
    {
    case kNewGame:
        controller.NewGame();
        break;
    case kUndo:
        controller.Undo();
        break;
    case kHint:
        controller.AskHint();
        break;
    case kEasy:
        controller.SetLevel(Level::kEasy);
        break;
    case kMedium:
        controller.SetLevel(Level::kMedium);
        break;
    case kHard:
        controller.SetLevel(Level::kHard);
        break;
    case kPerfect:
        controller.SetLevel(Level::kPerfect);
        break;
    case kRedPucks:
    case kYellowPucks:
        controller.SetPersonRed(command == kRedPucks);
        break;
    case kPersonStarts:
    case kComputerStarts:
        controller.SetPersonStarts(command == kPersonStarts);
        break;
    case kAlternate:
        controller.SetAlternate(!controller.settings().does_alternate);
        break;
    case kAgainstComputer:
    case kAgainstFriend:
        controller.SetTwoPlayer(command == kAgainstFriend);
        break;
    case kSound:
        controller.SetSound(!controller.settings().has_sound);
        break;
    case kMarks:
        controller.SetMarks(!controller.settings().has_marks);
        break;
    case kLarger:
        size_window(window.larger);
        break;
    case kSmaller:
        size_window(window.smaller);
        break;
    case kAbout:
        if (handlers.about)
        {
            handlers.about();
        }
        break;
    case kQuit:
        if (handlers.quit)
        {
            handlers.quit();
        }
        break;
    default:
        break;
    }
}

} // namespace knect4::app
