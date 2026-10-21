#include "app/controller.h"
#include "app/menus.h"

#include <gtest/gtest.h>

#include <chrono>
#include <thread>
#include <vector>

namespace knect4::app
{
namespace
{

/** The clicks heard. */
class Clicks final : public ClickSink
{
  public:
    /** A click heard. @param landing On what. @param gain How loud. */
    void Click(LandingOn landing, float gain) override
    {
        heard.push_back({landing, gain});
    }

    /** A puck has started to fall. */
    void Prepare() override
    {
        ++prepares;
    }

    int prepares{0}; ///< The falls made ready for.

    /** One click. */
    struct Heard
    {
        LandingOn landing; ///< On what.
        float     gain;    ///< How loud.
    };

    std::vector<Heard> heard; ///< Every click, in order.
};

/**
 * Frames of a sixtieth of a second until `done`, or a generous limit; the computer's search runs on its own thread,
 * so a frame waits a moment when it is thinking.
 * @param controller The game.
 * @param done When to stop.
 * @return Whether `done` came true.
 */
template <typename Done> bool RunUntil(Controller &controller, Done done)
{
    const auto give_up = std::chrono::steady_clock::now() + std::chrono::seconds(60);
    while (!done())
    {
        if (std::chrono::steady_clock::now() > give_up)
        {
            return false;
        }
        if (controller.phase() == Phase::kComputerThinking)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        controller.Frame(1.0 / 60.0);
    }
    return true;
}

TEST(Controller, StartsWithThePersonToMoveByDefault)
{
    Controller controller(nullptr);
    EXPECT_EQ(controller.phase(), Phase::kPersonToMove);
    EXPECT_EQ(controller.StatusText(), "Your move");
    EXPECT_EQ(controller.MovesMade(), 0);
    EXPECT_TRUE(controller.Picture().may_choose);
}

TEST(Controller, DropsThePersonsPuckWhichFallsBehindTheFrameAndClicksAsItLands)
{
    Clicks     clicks;
    Controller controller(&clicks);
    ASSERT_TRUE(controller.Drop(3));
    EXPECT_EQ(controller.phase(), Phase::kDropping);
    EXPECT_FALSE(controller.Drop(4)); // one at a time

    // While it falls it is not at rest on the board, and nothing is held.
    controller.Frame(1.0 / 60.0);
    BoardPicture falling = controller.Picture();
    ASSERT_TRUE(falling.falling.has_value());
    EXPECT_EQ(falling.falling->column, 3);
    EXPECT_EQ(falling.falling->player, kMin);
    EXPECT_EQ(falling.At(3, 0), kEmpty);
    EXPECT_FALSE(falling.held.has_value());
    EXPECT_TRUE(clicks.heard.empty());

    ASSERT_TRUE(RunUntil(controller, [&] { return controller.phase() != Phase::kDropping; }));
    EXPECT_EQ(controller.game().At(3, 0), kMin);
    EXPECT_EQ(controller.Picture().At(3, 0), kMin);
    // A loud first click on the base, softer bounces after.
    ASSERT_GE(clicks.heard.size(), 2u);
    EXPECT_EQ(clicks.heard[0].landing, LandingOn::kBase);
    EXPECT_GT(clicks.heard[0].gain, 0.9f);
    EXPECT_LT(clicks.heard[1].gain, clicks.heard[0].gain);
    EXPECT_EQ(controller.phase(), Phase::kComputerThinking);
}

TEST(Controller, PacesTheComputersTurnAndPlaysTheSearchsMove)
{
    Settings settings;
    settings.level             = Level::kEasy;
    settings.does_person_start = false;
    Controller controller(nullptr, settings);
    EXPECT_EQ(controller.phase(), Phase::kComputerThinking);
    EXPECT_EQ(controller.StatusText(), "Thinking…");

    // However fast the search, the turn takes its least time before the puck moves.
    double elapsed = 0.0;
    ASSERT_TRUE(RunUntil(controller, [&] {
        elapsed += 1.0 / 60.0;
        return controller.phase() == Phase::kDropping;
    }));
    EXPECT_GE(elapsed, Controller::kLeastThinking);

    // Its puck lands, and it is the person's turn.
    ASSERT_TRUE(RunUntil(controller, [&] { return controller.phase() == Phase::kPersonToMove; }));
    ASSERT_EQ(controller.MovesMade(), 1);
    EXPECT_EQ(controller.game().At(controller.moves()[0], 0), kMax);
}

TEST(Controller, EndsTheGameWithTheLineLitAndScoresIt)
{
    Controller controller(nullptr);
    // The person (first) has three across the bottom; the computer three above them.
    controller.PlaceMoves({0, 0, 1, 1, 2, 2});
    ASSERT_EQ(controller.phase(), Phase::kPersonToMove);
    ASSERT_TRUE(controller.Drop(3));
    ASSERT_TRUE(RunUntil(controller, [&] { return controller.IsOver(); }));
    EXPECT_EQ(controller.phase(), Phase::kWon);
    EXPECT_EQ(controller.score().min_wins, 1);
    const BoardPicture picture = controller.Picture();
    EXPECT_EQ(picture.winning.size(), 4u);
    EXPECT_EQ(picture.banner, "You win!");
    EXPECT_FALSE(picture.may_choose);
    EXPECT_FALSE(controller.Drop(4));

    // A new game keeps the score.
    controller.NewGame();
    EXPECT_EQ(controller.MovesMade(), 0);
    EXPECT_EQ(controller.score().min_wins, 1);
    EXPECT_TRUE(controller.Picture().banner.empty());
}

TEST(Controller, CountsTheComputersWinAndADraw)
{
    Controller controller(nullptr);
    controller.PlaceMoves({0, 3, 0, 3, 1, 3, 6, 3});
    EXPECT_EQ(controller.phase(), Phase::kWon);
    EXPECT_EQ(controller.StatusText(), "I win");
    EXPECT_EQ(controller.score().max_wins, 1);

    // A game played to a full board with no four: found by a search for a board with none whose pucks can be dropped
    // in turn.
    controller.PlaceMoves({2, 0, 2, 0, 0, 1, 0, 0, 4, 0, 4, 1, 1, 2, 1, 1, 5, 1, 5, 2, 2,
                           3, 2, 3, 3, 4, 3, 3, 6, 3, 6, 4, 4, 5, 4, 5, 5, 6, 5, 6, 6, 6});
    EXPECT_EQ(controller.MovesMade(), 42);
    EXPECT_EQ(controller.phase(), Phase::kDrawn);
    EXPECT_EQ(controller.StatusText(), "A draw");
    EXPECT_EQ(controller.score().drawn, 1);
    EXPECT_EQ(controller.score().max_wins, 1);
}

TEST(Controller, HoldsThePersonsPuckOverTheColumnUnderThePointer)
{
    Controller controller(nullptr);
    EXPECT_FALSE(controller.Picture().held.has_value());
    controller.Hover(5);
    for (int frame = 0; frame != 30; ++frame)
    {
        controller.Frame(1.0 / 60.0);
    }
    BoardPicture picture = controller.Picture();
    ASSERT_TRUE(picture.held.has_value());
    EXPECT_NEAR(picture.held->column, 5.0f, 0.05f);
    EXPECT_EQ(picture.held->landing_column, 5);
    EXPECT_FLOAT_EQ(picture.held->opacity, 1.0f);

    // The keys move it a column at a time, stopping at the edge, and drop it there.
    controller.Step(1);
    controller.Step(1);
    EXPECT_EQ(controller.Picture().held->landing_column, 6);
    controller.Step(-1);
    EXPECT_TRUE(controller.DropHeld());
    ASSERT_TRUE(RunUntil(controller, [&] { return controller.phase() != Phase::kDropping; }));
    EXPECT_EQ(controller.game().At(5, 0), kMin);
}

TEST(Controller, StepsPastAFullColumn)
{
    Controller controller(nullptr);
    // Column 4 filled to the top; the person to move.
    controller.PlaceMoves({4, 4, 4, 4, 4, 4});
    ASSERT_EQ(controller.phase(), Phase::kPersonToMove);
    controller.Hover(3);
    controller.Frame(0.25);
    ASSERT_TRUE(controller.Picture().held.has_value());
    EXPECT_EQ(controller.Picture().held->landing_column, 3);
    controller.Step(1);
    ASSERT_TRUE(controller.Picture().held.has_value());
    EXPECT_EQ(controller.Picture().held->landing_column, 5);
    controller.Step(-1);
    EXPECT_EQ(controller.Picture().held->landing_column, 3);
    EXPECT_FALSE(controller.Drop(4));

    // Off the board, the held puck fades away.
    controller.Hover(-1);
    for (int frame = 0; frame != 30; ++frame)
    {
        controller.Frame(1.0 / 60.0);
    }
    EXPECT_FALSE(controller.Picture().held.has_value());
}

TEST(Controller, StopsAtTheEdgeRatherThanWrapping)
{
    Controller controller(nullptr);
    // Column 6 full: a step right from 5 stays at 5; column 0 full: a step left from 1 stays at 1.
    controller.PlaceMoves({6, 6, 6, 6, 6, 6, 0, 0, 0, 0, 0, 0});
    ASSERT_EQ(controller.phase(), Phase::kPersonToMove);
    controller.Hover(5);
    controller.Frame(0.25);
    controller.Step(1);
    ASSERT_TRUE(controller.Picture().held.has_value());
    EXPECT_EQ(controller.Picture().held->landing_column, 5);
    controller.Hover(1);
    controller.Step(-1);
    EXPECT_EQ(controller.Picture().held->landing_column, 1);
}

TEST(Controller, MovesAPuckHeldOverAFilledColumnToTheNearestWithRoom)
{
    // The person's puck was held over column 3 when it filled: the next turn holds it over 4, the nearer with room.
    Controller controller(nullptr);
    controller.Hover(3);
    controller.PlaceMoves({3, 3, 3, 3, 3, 3});
    ASSERT_EQ(controller.phase(), Phase::kPersonToMove);
    controller.Frame(0.25);
    ASSERT_TRUE(controller.Picture().held.has_value());
    EXPECT_EQ(controller.Picture().held->landing_column, 4);
    EXPECT_TRUE(controller.DropHeld());
}

TEST(Controller, ClicksAPuckLandingOnAnotherAndMakesReadyAsItFalls)
{
    Clicks     clicks;
    Controller controller(&clicks);
    controller.PlaceMoves({2, 5});
    ASSERT_TRUE(controller.Drop(2));
    EXPECT_EQ(clicks.prepares, 1);
    ASSERT_TRUE(RunUntil(controller, [&] { return controller.phase() != Phase::kDropping; }));
    ASSERT_FALSE(clicks.heard.empty());
    EXPECT_EQ(clicks.heard[0].landing, LandingOn::kPuck);
    // From one row up, a shorter fall: a softer first click than one to the base.
    EXPECT_LT(clicks.heard[0].gain, 1.0f);

    // With the sound off, nothing is made ready.
    controller.SetSound(false);
    controller.PlaceMoves({});
    ASSERT_TRUE(controller.Drop(0));
    EXPECT_EQ(clicks.prepares, 1);
}

TEST(Controller, LetsTheComputerWinThroughItsOwnTurn)
{
    // The computer, starting, has three in column 6 and the person three spread: it takes the fourth itself.
    Settings settings;
    settings.level             = Level::kEasy;
    settings.does_person_start = false;
    Controller controller(nullptr, settings);
    controller.PlaceMoves({6, 0, 6, 2, 6, 4});
    ASSERT_EQ(controller.phase(), Phase::kComputerThinking);
    ASSERT_TRUE(RunUntil(controller, [&] { return controller.IsOver(); }));
    EXPECT_EQ(controller.winner(), kMax);
    EXPECT_EQ(controller.game().At(6, 3), kMax);
    EXPECT_EQ(controller.score().max_wins, 1);
    EXPECT_EQ(controller.StatusText(), "I win");
}

TEST(Controller, StopsAThinkingComputerForANewGame)
{
    Settings settings;
    settings.level             = Level::kHard;
    settings.does_person_start = false;
    Controller controller(nullptr, settings);
    controller.SetPersonStarts(true);
    // The hard search is stopped and waited for: at once, not when it would have finished.
    const auto start = std::chrono::steady_clock::now();
    controller.NewGame();
    EXPECT_LT(std::chrono::steady_clock::now() - start, std::chrono::seconds(2));
    EXPECT_EQ(controller.phase(), Phase::kPersonToMove);
}

TEST(Controller, ChangesColoursAtOnceAndLevelForTheNextSearch)
{
    Controller controller(nullptr);
    EXPECT_EQ(controller.Picture().person.body, theme::kRed.body);
    controller.SetPersonRed(false);
    EXPECT_EQ(controller.Picture().person.body, theme::kYellow.body);
    EXPECT_EQ(controller.Picture().computer.body, theme::kRed.body);
    controller.SetLevel(Level::kHard);
    EXPECT_EQ(controller.settings().level, Level::kHard);
    EXPECT_EQ(StrengthOf(Level::kEasy), Strength::kEasy);
    EXPECT_EQ(StrengthOf(Level::kMedium), Strength::kMedium);
    EXPECT_EQ(StrengthOf(Level::kHard), Strength::kHard);
    EXPECT_EQ(StrengthOf(Level::kPerfect), Strength::kPerfect);
}

TEST(Controller, KeepsQuietWithTheSoundOff)
{
    Clicks     clicks;
    Controller controller(&clicks);
    controller.SetSound(false);
    controller.Drop(0);
    ASSERT_TRUE(RunUntil(controller, [&] { return controller.phase() != Phase::kDropping; }));
    EXPECT_TRUE(clicks.heard.empty());
}

TEST(Menus, TickTheSettingsAndCarryOutTheirCommands)
{
    Controller             controller(nullptr);
    reckless::app::MenuBar bar = BuildMenuBar(MenuStateOf(controller, {}));
    ASSERT_EQ(bar.menus.size(), 4u); // Game, Board, View, Help
    const auto find = [&bar](int command) {
        for (const reckless::app::MenuEntry &entry : bar.menus[0].submenu)
        {
            if (entry.id == command)
            {
                return entry;
            }
        }
        return reckless::app::MenuEntry{};
    };
    EXPECT_TRUE(find(kMedium).checked);
    EXPECT_FALSE(find(kHard).checked);
    EXPECT_TRUE(find(kSound).checked);

    RunCommand(kHard, controller, {});
    RunCommand(kYellowPucks, controller, {});
    RunCommand(kComputerStarts, controller, {});
    RunCommand(kSound, controller, {});
    EXPECT_EQ(controller.settings().level, Level::kHard);
    EXPECT_FALSE(controller.settings().is_person_red);
    EXPECT_FALSE(controller.settings().does_person_start);
    EXPECT_FALSE(controller.settings().has_sound);
    bar = BuildMenuBar(MenuStateOf(controller, {}));
    EXPECT_TRUE(find(kHard).checked);
    EXPECT_TRUE(find(kYellowPucks).checked);

    bool            has_quit = false;
    CommandHandlers handlers;
    handlers.quit = [&has_quit] { has_quit = true; };
    RunCommand(kQuit, controller, handlers);
    EXPECT_TRUE(has_quit);
    RunCommand(kNewGame, controller, {});
    EXPECT_EQ(controller.phase(), Phase::kComputerThinking);
}

/** @return Choices for a game between two people, Red starting. */
Settings TwoPlayers()
{
    Settings settings;
    settings.is_two_player = true;
    return settings;
}

TEST(TwoPlayers, TakeTurnsWithTheHandInEachOnesColour)
{
    Clicks     clicks;
    Controller controller(&clicks, TwoPlayers());
    EXPECT_EQ(controller.phase(), Phase::kPersonToMove);
    EXPECT_EQ(controller.mover(), kMin);
    EXPECT_EQ(controller.StatusText(), "Red to move");
    EXPECT_EQ(controller.Picture().person.body, theme::kRed.body);
    EXPECT_EQ(controller.Picture().computer.body, theme::kYellow.body);

    // Red drops; once it lands it is Yellow's turn, a person's again - no search, no computer.
    ASSERT_TRUE(controller.Drop(3));
    ASSERT_TRUE(RunUntil(controller, [&] { return controller.phase() != Phase::kDropping; }));
    EXPECT_FALSE(clicks.heard.empty());
    EXPECT_EQ(controller.phase(), Phase::kPersonToMove);
    EXPECT_EQ(controller.mover(), kMax);
    EXPECT_EQ(controller.StatusText(), "Yellow to move");
    controller.Hover(4);
    controller.Frame(0.25);
    ASSERT_TRUE(controller.Picture().held.has_value());
    EXPECT_EQ(controller.Picture().held->player, kMax);

    // Yellow's puck lands Yellow's.
    ASSERT_TRUE(controller.Drop(4));
    ASSERT_TRUE(RunUntil(controller, [&] { return controller.phase() != Phase::kDropping; }));
    EXPECT_EQ(controller.game().At(3, 0), kMin);
    EXPECT_EQ(controller.game().At(4, 0), kMax);
    EXPECT_EQ(controller.mover(), kMin);
}

TEST(TwoPlayers, NameTheWinnerByColourAndKeepAScoreOfTheirOwn)
{
    Controller controller(nullptr, TwoPlayers());
    // Red across the bottom, Yellow on top of each; Yellow's fourth would come next, but Red's comes first.
    controller.PlaceMoves({0, 0, 1, 1, 2, 2});
    ASSERT_EQ(controller.mover(), kMin);
    ASSERT_TRUE(controller.Drop(3));
    ASSERT_TRUE(RunUntil(controller, [&] { return controller.IsOver(); }));
    EXPECT_EQ(controller.phase(), Phase::kWon);
    EXPECT_EQ(controller.winner(), kMin);
    EXPECT_EQ(controller.StatusText(), "Red wins!");
    EXPECT_EQ(controller.Picture().banner, "Red wins!");
    EXPECT_EQ(controller.Picture().banner_colour, theme::kRed.light);
    EXPECT_EQ(controller.score().min_wins, 1);

    // Yellow's win, counted for Yellow.
    controller.PlaceMoves({0, 6, 0, 6, 1, 6, 2, 6});
    EXPECT_EQ(controller.winner(), kMax);
    EXPECT_EQ(controller.StatusText(), "Yellow wins!");
    EXPECT_EQ(controller.score().max_wins, 1);

    // The games against the computer are counted apart.
    controller.SetTwoPlayer(false);
    EXPECT_EQ(controller.score().min_wins, 0);
    EXPECT_EQ(controller.score().max_wins, 0);
    controller.SetTwoPlayer(true);
    EXPECT_EQ(controller.score().min_wins, 1);
}

TEST(TwoPlayers, LetYellowStartAndBeginAfreshWhenTheOpponentChanges)
{
    Settings settings          = TwoPlayers();
    settings.does_person_start = false;
    Controller controller(nullptr, settings);
    EXPECT_EQ(controller.phase(), Phase::kPersonToMove);
    EXPECT_EQ(controller.mover(), kMax);
    EXPECT_EQ(controller.StatusText(), "Yellow to move");

    // A game under way is left for a new one when the opponent changes - its pucks let out first - and against the
    // computer, the computer starts.
    ASSERT_TRUE(controller.Drop(2));
    controller.SetTwoPlayer(false);
    EXPECT_EQ(controller.MovesMade(), 0);
    EXPECT_EQ(controller.phase(), Phase::kEmptying);
    ASSERT_TRUE(RunUntil(controller, [&] { return controller.phase() != Phase::kEmptying; }));
    EXPECT_EQ(controller.phase(), Phase::kComputerThinking);
    // Choosing what is already chosen leaves the game alone.
    controller.SetTwoPlayer(false);
    EXPECT_EQ(controller.phase(), Phase::kComputerThinking);
}

TEST(TwoPlayers, IgnoreTheComputerChoicesInTheirColours)
{
    // The person's colour choice is the computer game's; between two people Red is always kMin.
    Settings settings      = TwoPlayers();
    settings.is_person_red = false;
    Controller controller(nullptr, settings);
    EXPECT_EQ(controller.ColoursOf(kMin).body, theme::kRed.body);
    controller.SetTwoPlayer(false);
    EXPECT_EQ(controller.ColoursOf(kMin).body, theme::kYellow.body);
}

TEST(Menus, OfferAFriendAndDimWhatTwoPlayersDoNotUse)
{
    Controller controller(nullptr);
    const auto find = [](const reckless::app::MenuBar &bar, int command) {
        for (const reckless::app::MenuEntry &entry : bar.menus[0].submenu)
        {
            if (entry.id == command)
            {
                return entry;
            }
        }
        return reckless::app::MenuEntry{};
    };
    reckless::app::MenuBar bar = BuildMenuBar(MenuStateOf(controller, {}));
    EXPECT_TRUE(find(bar, kAgainstComputer).checked);
    EXPECT_TRUE(find(bar, kEasy).enabled);
    EXPECT_EQ(find(bar, kPersonStarts).title, "You start");

    RunCommand(kAgainstFriend, controller, {});
    EXPECT_TRUE(controller.settings().is_two_player);
    bar = BuildMenuBar(MenuStateOf(controller, {}));
    EXPECT_TRUE(find(bar, kAgainstFriend).checked);
    EXPECT_FALSE(find(bar, kEasy).enabled);
    EXPECT_FALSE(find(bar, kRedPucks).enabled);
    EXPECT_EQ(find(bar, kPersonStarts).title, "Red starts");
    EXPECT_EQ(find(bar, kComputerStarts).title, "Yellow starts");

    RunCommand(kAgainstComputer, controller, {});
    EXPECT_FALSE(controller.settings().is_two_player);
}

} // namespace
} // namespace knect4::app

namespace knect4::app
{
namespace
{

TEST(Controller, IgnoresStepsAndDropsOutOfTurn)
{
    Controller controller(nullptr);
    controller.Hover(2);
    ASSERT_TRUE(controller.Drop(2));
    controller.Step(1); // the puck is falling: no one holds one
    EXPECT_FALSE(controller.Drop(3));
    EXPECT_FALSE(controller.DropHeld());
    EXPECT_EQ(controller.MovesMade(), 1);
}

TEST(Controller, PlacesOnlyTheMovesThatFit)
{
    Controller controller(nullptr);
    // Seven into a column of six: the seventh is left out, and after six it is the person's turn again.
    controller.PlaceMoves({0, 0, 0, 0, 0, 0, 0});
    EXPECT_EQ(controller.MovesMade(), 6);
    EXPECT_EQ(controller.game().ColumnHeight(0), 6);
    EXPECT_EQ(controller.phase(), Phase::kPersonToMove);

    // No moves with the computer starting: its search starts afresh.
    Settings computer_first;
    computer_first.does_person_start = false;
    Controller second(nullptr, computer_first);
    second.PlaceMoves({});
    EXPECT_EQ(second.phase(), Phase::kComputerThinking);
    EXPECT_EQ(second.StatusText(), "Thinking…");
}

TEST(Controller, ShowsTheComputersPuckAsItAimsAndFalls)
{
    Settings settings;
    settings.level             = Level::kEasy;
    settings.does_person_start = false;
    Controller controller(nullptr, settings);
    ASSERT_TRUE(RunUntil(controller, [&] { return controller.phase() != Phase::kComputerThinking; }));
    if (controller.phase() == Phase::kComputerAiming)
    {
        const BoardPicture aiming = controller.Picture();
        ASSERT_TRUE(aiming.held.has_value());
        EXPECT_EQ(aiming.held->player, kMax);
        EXPECT_EQ(controller.StatusText(), "Thinking…");
    }
    ASSERT_TRUE(RunUntil(controller, [&] { return controller.phase() == Phase::kDropping; }));
    EXPECT_EQ(controller.StatusText(), "Thinking…");
    EXPECT_FALSE(controller.Picture().held.has_value());
}

TEST(Controller, GlowsAfterTheGameAndColoursTheBannerByWhoWon)
{
    Controller controller(nullptr);
    controller.PlaceMoves({0, 3, 0, 3, 1, 3, 6, 3});
    ASSERT_EQ(controller.winner(), kMax);
    const float first = controller.Picture().glow;
    controller.Frame(0.3);
    EXPECT_NE(controller.Picture().glow, first);
    EXPECT_EQ(controller.Picture().banner_colour, theme::kBad);
    EXPECT_EQ(controller.Picture().winning.size(), 4u);

    controller.PlaceMoves({0, 0, 1, 1, 2, 2, 3});
    EXPECT_EQ(controller.Picture().banner_colour, theme::kGood);

    controller.PlaceMoves({2, 0, 2, 0, 0, 1, 0, 0, 4, 0, 4, 1, 1, 2, 1, 1, 5, 1, 5, 2, 2,
                           3, 2, 3, 3, 4, 3, 3, 6, 3, 6, 4, 4, 5, 4, 5, 5, 6, 5, 6, 6, 6});
    EXPECT_EQ(controller.Picture().banner_colour, theme::kInk);
    EXPECT_TRUE(controller.Picture().winning.empty());
    EXPECT_EQ(controller.winner(), kEmpty);
}

TEST(TwoPlayers, NameWhoseTurnWhileAPuckFallsAndCallADraw)
{
    Settings two;
    two.is_two_player = true;
    Controller controller(nullptr, two);
    ASSERT_TRUE(controller.Drop(3));
    EXPECT_EQ(controller.StatusText(), "Red to move");
    ASSERT_TRUE(RunUntil(controller, [&] { return controller.phase() != Phase::kDropping; }));
    ASSERT_TRUE(controller.Drop(3));
    EXPECT_EQ(controller.StatusText(), "Yellow to move");

    controller.PlaceMoves({2, 0, 2, 0, 0, 1, 0, 0, 4, 0, 4, 1, 1, 2, 1, 1, 5, 1, 5, 2, 2,
                           3, 2, 3, 3, 4, 3, 3, 6, 3, 6, 4, 4, 5, 4, 5, 5, 6, 5, 6, 6, 6});
    EXPECT_EQ(controller.phase(), Phase::kDrawn);
    EXPECT_EQ(controller.StatusText(), "A draw");
    EXPECT_EQ(controller.score().drawn, 1);
}

TEST(Menus, CarryOutEveryChoice)
{
    Controller controller(nullptr);
    RunCommand(kEasy, controller, {});
    EXPECT_EQ(controller.settings().level, Level::kEasy);
    RunCommand(kMedium, controller, {});
    EXPECT_EQ(controller.settings().level, Level::kMedium);
    RunCommand(kYellowPucks, controller, {});
    RunCommand(kRedPucks, controller, {});
    EXPECT_TRUE(controller.settings().is_person_red);
    RunCommand(kComputerStarts, controller, {});
    RunCommand(kPersonStarts, controller, {});
    EXPECT_TRUE(controller.settings().does_person_start);
    RunCommand(kSound, controller, {});
    RunCommand(kSound, controller, {});
    EXPECT_TRUE(controller.settings().has_sound);
    RunCommand(kQuit, controller, {}); // no one to tell: nothing happens
    RunCommand(kNoCommand, controller, {});
    RunCommand(999, controller, {});
    EXPECT_EQ(controller.phase(), Phase::kPersonToMove);
}

TEST(Controller, MakesNoDemandsOfASinkThatOnlyClicks)
{
    // A sink that only hears clicks takes the default of making nothing ready.
    class OnlyClicks final : public ClickSink
    {
      public:
        void Click(LandingOn landing, float gain) override
        {
            static_cast<void>(landing);
            static_cast<void>(gain);
            ++count;
        }

        int count{0}; ///< Clicks heard.
    } clicks;

    Controller controller(&clicks);
    ASSERT_TRUE(controller.Drop(0));
    ASSERT_TRUE(RunUntil(controller, [&] { return controller.phase() != Phase::kDropping; }));
    EXPECT_GT(clicks.count, 0);
}

} // namespace
} // namespace knect4::app

namespace knect4::app
{
namespace
{

TEST(Undo, TakesBackThePersonsMoveAndTheComputersAnswer)
{
    Controller controller(nullptr);
    EXPECT_FALSE(controller.CanUndo()); // nothing played
    controller.PlaceMoves({3, 2, 4, 1});
    ASSERT_EQ(controller.phase(), Phase::kPersonToMove);
    ASSERT_TRUE(controller.CanUndo());
    ASSERT_TRUE(controller.Undo());
    EXPECT_EQ(controller.moves(), (std::vector<int>{3, 2}));
    EXPECT_EQ(controller.game().At(4, 0), kEmpty);
    EXPECT_EQ(controller.game().At(1, 0), kEmpty);
    EXPECT_EQ(controller.phase(), Phase::kPersonToMove);
    ASSERT_TRUE(controller.Undo());
    EXPECT_TRUE(controller.moves().empty());
    EXPECT_FALSE(controller.Undo());
}

TEST(Undo, StopsTheThinkingComputerAndTakesBackOnlyThePersonsMove)
{
    Settings settings;
    settings.level = Level::kHard;
    Controller controller(nullptr, settings);
    controller.PlaceMoves({3});
    ASSERT_EQ(controller.phase(), Phase::kComputerThinking);
    const auto start = std::chrono::steady_clock::now();
    ASSERT_TRUE(controller.Undo());
    EXPECT_LT(std::chrono::steady_clock::now() - start, std::chrono::seconds(2));
    EXPECT_TRUE(controller.moves().empty());
    EXPECT_EQ(controller.phase(), Phase::kPersonToMove);
}

TEST(Undo, CannotTakeBackTheComputersOpeningAlone)
{
    Settings settings;
    settings.does_person_start = false;
    Controller controller(nullptr, settings);
    controller.PlaceMoves({3});
    ASSERT_EQ(controller.phase(), Phase::kPersonToMove);
    EXPECT_FALSE(controller.CanUndo()); // nothing of the person's to take back
    EXPECT_FALSE(controller.Undo());
}

TEST(Undo, UnscoresAGameItTakesBackAndRefusesWhileAPuckFalls)
{
    Controller controller(nullptr);
    controller.PlaceMoves({0, 0, 1, 1, 2, 2, 3});
    ASSERT_EQ(controller.phase(), Phase::kWon);
    ASSERT_EQ(controller.score().min_wins, 1);
    ASSERT_TRUE(controller.Undo()); // the winning move itself, the person's own
    EXPECT_EQ(controller.score().min_wins, 0);
    EXPECT_EQ(controller.phase(), Phase::kPersonToMove);
    EXPECT_TRUE(controller.Picture().winning.empty());
    EXPECT_EQ(controller.MovesMade(), 6);

    // The computer's win: taken back with the person's move before it.
    controller.PlaceMoves({0, 3, 0, 3, 1, 3, 6, 3});
    ASSERT_EQ(controller.winner(), kMax);
    ASSERT_TRUE(controller.Undo());
    EXPECT_EQ(controller.score().max_wins, 0);
    EXPECT_EQ(controller.MovesMade(), 6);

    // A draw, taken back between two people one move at a time.
    controller.SetTwoPlayer(true);
    controller.PlaceMoves({2, 0, 2, 0, 0, 1, 0, 0, 4, 0, 4, 1, 1, 2, 1, 1, 5, 1, 5, 2, 2,
                           3, 2, 3, 3, 4, 3, 3, 6, 3, 6, 4, 4, 5, 4, 5, 5, 6, 5, 6, 6, 6});
    ASSERT_EQ(controller.phase(), Phase::kDrawn);
    ASSERT_TRUE(controller.Undo());
    EXPECT_EQ(controller.score().drawn, 0);
    EXPECT_EQ(controller.MovesMade(), 41);
    EXPECT_EQ(controller.mover(), kMax); // Yellow played the 42nd

    // Not while a puck falls.
    ASSERT_TRUE(controller.Drop(6));
    EXPECT_FALSE(controller.CanUndo());
    EXPECT_FALSE(controller.Undo());
}

TEST(Hint, FindsTheWinningColumnAndClearsWhenAMoveIsMade)
{
    Controller controller(nullptr);
    controller.PlaceMoves({0, 6, 1, 6, 2, 5});
    ASSERT_EQ(controller.phase(), Phase::kPersonToMove);
    ASSERT_TRUE(controller.CanHint());
    ASSERT_TRUE(controller.AskHint());
    EXPECT_TRUE(controller.IsHintPending());
    EXPECT_FALSE(controller.CanHint());
    EXPECT_FALSE(controller.AskHint());
    EXPECT_EQ(controller.StatusText(), "Looking for a hint…");
    ASSERT_TRUE(RunUntil(controller, [&] {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        return controller.hint_column() >= 0;
    }));
    EXPECT_EQ(controller.hint_column(), 3);
    EXPECT_EQ(controller.Picture().hint_column, 3);
    EXPECT_FALSE(controller.IsHintPending());
    ASSERT_TRUE(controller.Drop(3));
    EXPECT_EQ(controller.hint_column(), -1);
}

TEST(Hint, IsGivenUpWhenThePersonMovesFirst)
{
    Controller controller(nullptr);
    ASSERT_TRUE(controller.AskHint());
    ASSERT_TRUE(controller.Drop(3)); // the search stopped and its answer thrown away
    EXPECT_FALSE(controller.IsHintPending());
    EXPECT_EQ(controller.hint_column(), -1);
    // Not on the computer's turn.
    ASSERT_TRUE(RunUntil(controller, [&] { return controller.phase() != Phase::kDropping; }));
    EXPECT_FALSE(controller.CanHint());
}

TEST(NewGame, LetsThePucksOutThroughTheSliderWithAClatter)
{
    Clicks     clicks;
    Controller controller(&clicks);
    controller.PlaceMoves({0, 1, 2, 3, 0, 1});
    controller.NewGame();
    EXPECT_EQ(controller.phase(), Phase::kEmptying);
    EXPECT_EQ(controller.MovesMade(), 0);
    EXPECT_EQ(controller.StatusText(), "New game");
    EXPECT_EQ(clicks.prepares, 1);
    controller.NewGame(); // already on its way
    EXPECT_EQ(controller.phase(), Phase::kEmptying);

    // The slider opens and the pucks fall out, fading as they go.
    controller.Frame(0.2);
    BoardPicture opening = controller.Picture();
    EXPECT_GT(opening.slide, 0.9f);
    EXPECT_EQ(opening.emptying.size(), 6u);
    for (int frame = 0; frame != 12; ++frame)
    {
        controller.Frame(1.0 / 60.0);
    }
    const BoardPicture falling = controller.Picture();
    ASSERT_FALSE(falling.emptying.empty());
    EXPECT_LT(falling.emptying.front().row, 0.0f);
    EXPECT_TRUE(falling.pieces.size() == 42u);
    EXPECT_EQ(falling.At(0, 0), kEmpty); // the board itself is already empty

    ASSERT_TRUE(RunUntil(controller, [&] { return controller.phase() != Phase::kEmptying; }));
    EXPECT_EQ(controller.phase(), Phase::kPersonToMove);
    EXPECT_TRUE(controller.Picture().emptying.empty());
    EXPECT_EQ(controller.Picture().slide, 0.0f);
    // A clatter for each column that held pucks.
    EXPECT_EQ(clicks.heard.size(), 4u);
}

TEST(BoardSize, ChangesTheBoardAndBeginsANewGameOnIt)
{
    Controller controller(nullptr);
    controller.PlaceMoves({3, 3});
    controller.SetBoardSize({16, 8});
    EXPECT_EQ(controller.settings().board, (BoardSize{16, 8}));
    EXPECT_EQ(controller.game().width(), 16);
    EXPECT_EQ(controller.game().height(), 8);
    EXPECT_EQ(controller.MovesMade(), 0);
    EXPECT_EQ(controller.phase(), Phase::kPersonToMove);
    const BoardPicture picture = controller.Picture();
    EXPECT_EQ(picture.layout.columns(), 16);
    EXPECT_EQ(picture.pieces.size(), 128u);
    EXPECT_EQ(controller.layout(), (art::Layout{16, 8}));
    // A size not offered, and the same size again, change nothing.
    controller.Drop(15);
    controller.SetBoardSize({5, 5});
    controller.SetBoardSize({16, 8});
    EXPECT_EQ(controller.MovesMade(), 1);
    // Settings that name a size not offered start on the standard board.
    Settings odd;
    odd.board = {5, 5};
    EXPECT_EQ(Controller(nullptr, odd).settings().board, kStandardBoard);
}

TEST(BoardSize, PlaysTheComputerOnTheSupersizeBoard)
{
    Settings settings;
    settings.board             = {16, 8};
    settings.level             = Level::kPerfect;
    settings.does_person_start = false;
    Controller controller(nullptr, settings);
    ASSERT_TRUE(RunUntil(controller, [&] { return controller.phase() == Phase::kPersonToMove; }));
    ASSERT_EQ(controller.MovesMade(), 1);
    EXPECT_GE(controller.moves()[0], 0);
    EXPECT_LT(controller.moves()[0], 16);
    // Keys 1 to 9 reach the first nine columns; the arrows the others.
    controller.Hover(15);
    EXPECT_TRUE(controller.DropHeld());
}

TEST(TwoPlayers, AlternateWhoStartsAndUseTheirNames)
{
    Settings two;
    two.is_two_player = true;
    Controller controller(nullptr, two);
    controller.SetNames("Ann", "");
    EXPECT_EQ(controller.StatusText(), "Ann to move");
    controller.PlaceMoves({0, 6, 0, 6, 0, 6, 0});
    EXPECT_EQ(controller.StatusText(), "Ann wins!");
    controller.NewGame();
    ASSERT_TRUE(RunUntil(controller, [&] { return controller.phase() != Phase::kEmptying; }));
    // Taking turns: Yellow (named by colour) starts the second game.
    EXPECT_FALSE(controller.settings().does_person_start);
    EXPECT_EQ(controller.mover(), kMax);
    EXPECT_EQ(controller.StatusText(), "Yellow to move");
    // Not taking turns: the same side starts again.
    controller.SetAlternate(false);
    controller.Drop(3);
    controller.NewGame();
    ASSERT_TRUE(RunUntil(controller, [&] { return controller.phase() != Phase::kEmptying; }));
    EXPECT_EQ(controller.mover(), kMax);
}

TEST(Controller, MarksThePucksAndTheKeyboardsColumn)
{
    Controller controller(nullptr);
    EXPECT_FALSE(controller.Picture().has_marks);
    controller.SetMarks(true);
    EXPECT_TRUE(controller.Picture().has_marks);
    controller.Step(1);
    controller.Frame(0.25);
    ASSERT_TRUE(controller.Picture().held.has_value());
    EXPECT_TRUE(controller.Picture().held->is_keyboard);
    controller.Hover(2);
    EXPECT_FALSE(controller.Picture().held->is_keyboard); // the pointer took it back
}

TEST(Controller, KeepsTheScoresItIsGivenAndTheWindowsSize)
{
    Scores scores;
    scores.computer   = {5, 6, 7};
    scores.two_player = {1, 2, 3};
    Controller controller(nullptr, {}, scores);
    EXPECT_EQ(controller.score(), scores.computer);
    controller.SetTwoPlayer(true);
    EXPECT_EQ(controller.score(), scores.two_player);
    EXPECT_EQ(controller.scores(), scores);
    controller.SetWindowPercent(75);
    EXPECT_EQ(controller.settings().window_percent, 75);
}

TEST(Controller, PlaysEveryLevelAgainstItself)
{
    // Each level answers on its own turn through the controller's thread, a short game each.
    for (const Level level : {Level::kEasy, Level::kMedium, Level::kHard, Level::kPerfect})
    {
        Settings settings;
        settings.level             = level;
        settings.does_person_start = false;
        Controller controller(nullptr, settings);
        ASSERT_TRUE(RunUntil(controller, [&] { return controller.phase() == Phase::kPersonToMove; }));
        EXPECT_EQ(controller.MovesMade(), 1);
    }
}

} // namespace
} // namespace knect4::app

namespace knect4::app
{
namespace
{

/**
 * @param menu A menu.
 * @param command A command.
 * @return The menu's entry for it; an empty one where it has none.
 */
reckless::app::MenuEntry EntryOf(const reckless::app::MenuEntry &menu, int command)
{
    for (const reckless::app::MenuEntry &entry : menu.submenu)
    {
        if (entry.id == command)
        {
            return entry;
        }
    }
    return {};
}

TEST(Menus, OfferTheBoardsTheWindowsSizesAndAbout)
{
    Controller     controller(nullptr);
    WindowSizeMenu window;
    window.has_window                = true;
    window.in_force                  = 100;
    window.fits                      = {true, true, true, true, false};
    window.larger                    = 125;
    window.smaller                   = 75;
    const reckless::app::MenuBar bar = BuildMenuBar(MenuStateOf(controller, window));
    ASSERT_EQ(bar.menus.size(), 4u);
    EXPECT_EQ(bar.menus[1].title, "Board");
    EXPECT_EQ(bar.menus[1].submenu.size(), kBoardSizes.size());
    EXPECT_TRUE(EntryOf(bar.menus[1], kBoardBase + 1).checked); // 7 x 6
    EXPECT_EQ(bar.menus[2].title, "View");
    EXPECT_TRUE(EntryOf(bar.menus[2], kWindowSizeBase + 100).checked);
    EXPECT_FALSE(EntryOf(bar.menus[2], kWindowSizeBase + 150).enabled); // the screen has no room for it
    EXPECT_TRUE(EntryOf(bar.menus[2], kLarger).enabled);
    EXPECT_FALSE(EntryOf(bar.menus[2], kMarks).checked);
    EXPECT_EQ(bar.about_id, kAbout);
    EXPECT_FALSE(EntryOf(bar.menus[0], kUndo).enabled);
    EXPECT_TRUE(EntryOf(bar.menus[0], kHint).enabled);
    EXPECT_FALSE(EntryOf(bar.menus[0], kAlternate).enabled); // only between two people

    // The commands that reach past the game go to their handlers; the rest to the controller.
    int             sized          = 0;
    bool            is_about_shown = false;
    CommandHandlers handlers;
    handlers.window_size = [&sized](int percent) { sized = percent; };
    handlers.about       = [&is_about_shown] { is_about_shown = true; };
    RunCommand(kLarger, controller, handlers, window);
    EXPECT_EQ(sized, 125);
    RunCommand(kSmaller, controller, handlers, window);
    EXPECT_EQ(sized, 75);
    RunCommand(kWindowSizeBase + 50, controller, handlers, window);
    EXPECT_EQ(sized, 50);
    RunCommand(kWindowSizeBase + 60, controller, handlers, window); // not a size offered
    EXPECT_EQ(sized, 50);
    RunCommand(kAbout, controller, handlers, window);
    EXPECT_TRUE(is_about_shown);
    RunCommand(kBoardBase + 4, controller, handlers, window);
    EXPECT_EQ(controller.settings().board, (BoardSize{16, 8}));
    RunCommand(kMarks, controller, handlers, window);
    EXPECT_TRUE(controller.settings().has_marks);
    RunCommand(kPerfect, controller, handlers, window);
    EXPECT_EQ(controller.settings().level, Level::kPerfect);
    RunCommand(kAgainstFriend, controller, handlers, window);
    RunCommand(kAlternate, controller, handlers, window);
    EXPECT_FALSE(controller.settings().does_alternate);
    controller.PlaceMoves({0, 1});
    RunCommand(kUndo, controller, handlers, window);
    EXPECT_EQ(controller.MovesMade(), 1);
    RunCommand(kHint, controller, handlers, window);
    EXPECT_TRUE(controller.IsHintPending() || controller.hint_column() >= 0);
    // With no handlers the commands past the game do nothing.
    RunCommand(kAbout, controller, {}, window);
    RunCommand(kLarger, controller, {}, window);
    RunCommand(kLarger, controller, handlers, WindowSizeMenu{}); // no size larger: nothing
    EXPECT_EQ(sized, 50);
}

} // namespace
} // namespace knect4::app
