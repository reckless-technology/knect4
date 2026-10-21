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
    EXPECT_EQ(falling.pieces[3][0], kEmpty);
    EXPECT_FALSE(falling.held.has_value());
    EXPECT_TRUE(clicks.heard.empty());

    ASSERT_TRUE(RunUntil(controller, [&] { return controller.phase() != Phase::kDropping; }));
    EXPECT_EQ(controller.game().At(3, 0), kMin);
    EXPECT_EQ(controller.Picture().pieces[3][0], kMin);
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

    // The move is the search's own.
    Game game;
    int  expected = -1;
    game.NextMove(DepthOf(Level::kEasy), kMax, expected);
    ASSERT_TRUE(RunUntil(controller, [&] { return controller.phase() == Phase::kPersonToMove; }));
    EXPECT_EQ(controller.game().At(expected, 0), kMax);
}

TEST(Controller, EndsTheGameWithTheLineLitAndScoresIt)
{
    Controller controller(nullptr);
    // The person (first) has three across the bottom; the computer three above them.
    controller.PlaceMoves({0, 0, 1, 1, 2, 2});
    ASSERT_EQ(controller.phase(), Phase::kPersonToMove);
    ASSERT_TRUE(controller.Drop(3));
    ASSERT_TRUE(RunUntil(controller, [&] { return controller.IsOver(); }));
    EXPECT_EQ(controller.phase(), Phase::kPersonWon);
    EXPECT_EQ(controller.score().person, 1);
    const BoardPicture picture = controller.Picture();
    EXPECT_EQ(picture.winning.size(), 4u);
    EXPECT_EQ(picture.banner, "You win!");
    EXPECT_FALSE(picture.may_choose);
    EXPECT_FALSE(controller.Drop(4));

    // A new game keeps the score.
    controller.NewGame();
    EXPECT_EQ(controller.MovesMade(), 0);
    EXPECT_EQ(controller.score().person, 1);
    EXPECT_TRUE(controller.Picture().banner.empty());
}

TEST(Controller, CountsTheComputersWinAndADraw)
{
    Controller controller(nullptr);
    controller.PlaceMoves({0, 3, 0, 3, 1, 3, 6, 3});
    EXPECT_EQ(controller.phase(), Phase::kComputerWon);
    EXPECT_EQ(controller.StatusText(), "I win");
    EXPECT_EQ(controller.score().computer, 1);

    // A game played to a full board with no four: found by a search for a board with none whose pucks can be dropped
    // in turn.
    controller.PlaceMoves({2, 0, 2, 0, 0, 1, 0, 0, 4, 0, 4, 1, 1, 2, 1, 1, 5, 1, 5, 2, 2,
                           3, 2, 3, 3, 4, 3, 3, 6, 3, 6, 4, 4, 5, 4, 5, 5, 6, 5, 6, 6, 6});
    EXPECT_EQ(controller.MovesMade(), 42);
    EXPECT_EQ(controller.phase(), Phase::kDrawn);
    EXPECT_EQ(controller.StatusText(), "A draw");
    EXPECT_EQ(controller.score().drawn, 1);
    EXPECT_EQ(controller.score().computer, 1);
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
    controller.Step(1);
    EXPECT_EQ(controller.Picture().held.has_value() ? controller.Picture().held->landing_column : 5, 5);
    EXPECT_FALSE(controller.Drop(4));

    // Off the board, the held puck fades away.
    controller.Hover(-1);
    for (int frame = 0; frame != 30; ++frame)
    {
        controller.Frame(1.0 / 60.0);
    }
    EXPECT_FALSE(controller.Picture().held.has_value());
}

TEST(Controller, StopsAThinkingComputerForANewGame)
{
    Settings settings;
    settings.level             = Level::kHard;
    settings.does_person_start = false;
    Controller controller(nullptr, settings);
    controller.SetPersonStarts(true);
    controller.NewGame(); // the hard search stopped and waited for
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
    EXPECT_EQ(DepthOf(Level::kEasy), 3);
    EXPECT_EQ(DepthOf(Level::kMedium), 7);
    EXPECT_EQ(DepthOf(Level::kHard), 11);
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
    reckless::app::MenuBar bar = BuildMenuBar(controller.settings());
    ASSERT_EQ(bar.menus.size(), 1u);
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
    bar = BuildMenuBar(controller.settings());
    EXPECT_TRUE(find(kHard).checked);
    EXPECT_TRUE(find(kYellowPucks).checked);

    bool has_quit = false;
    RunCommand(kQuit, controller, [&has_quit] { has_quit = true; });
    EXPECT_TRUE(has_quit);
    RunCommand(kNewGame, controller, {});
    EXPECT_EQ(controller.phase(), Phase::kComputerThinking);
}

} // namespace
} // namespace knect4::app
