#include "app/menus.h"

#include <string>
#include <utility>

namespace knect4::app
{

namespace
{

using reckless::app::MenuEntry;
using reckless::app::Shortcut;

/**
 * An item.
 * @param title Its text.
 * @param command Its command.
 * @param is_checked Whether it shows a tick.
 * @param shortcut Its key; none for no key.
 * @return The item.
 */
MenuEntry Item(std::string title, int command, bool is_checked = false, Shortcut shortcut = {})
{
    MenuEntry entry = MenuEntry::Item(std::move(title), command, shortcut);
    entry.checked   = is_checked;
    return entry;
}

} // namespace

reckless::app::MenuBar BuildMenuBar(const Settings &settings)
{
    return reckless::app::MenuBar{{MenuEntry::Menu(
        "Game", {Item("New game", kNewGame, false, Shortcut::Primary('n')), MenuEntry::Separator(),
                 Item("Easy", kEasy, settings.level == Level::kEasy, Shortcut::Primary('1')),
                 Item("Medium", kMedium, settings.level == Level::kMedium, Shortcut::Primary('2')),
                 Item("Hard", kHard, settings.level == Level::kHard, Shortcut::Primary('3')), MenuEntry::Separator(),
                 Item("Red pucks", kRedPucks, settings.is_person_red),
                 Item("Yellow pucks", kYellowPucks, !settings.is_person_red), MenuEntry::Separator(),
                 Item("You start", kPersonStarts, settings.does_person_start),
                 Item("Computer starts", kComputerStarts, !settings.does_person_start), MenuEntry::Separator(),
                 Item("Sound", kSound, settings.has_sound, Shortcut::Primary('m')), MenuEntry::Separator(),
                 Item("Quit", kQuit, false, Shortcut::Primary('q'))})}};
}

void RunCommand(int command, Controller &controller, const std::function<void()> &quit)
{
    switch (command)
    {
    case kNewGame:
        controller.NewGame();
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
    case kRedPucks:
    case kYellowPucks:
        controller.SetPersonRed(command == kRedPucks);
        break;
    case kPersonStarts:
    case kComputerStarts:
        controller.SetPersonStarts(command == kPersonStarts);
        break;
    case kSound:
        controller.SetSound(!controller.settings().has_sound);
        break;
    case kQuit:
        if (quit)
        {
            quit();
        }
        break;
    default:
        break;
    }
}

} // namespace knect4::app
