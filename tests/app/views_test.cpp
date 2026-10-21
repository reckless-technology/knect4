#include "app/board_art.h"
#include "app/board_view.h"
#include "app/controller.h"
#include "app/main_view.h"
#include "app/side_panel.h"
#include "app/theme.h"

#include "Button.h"
#include "Embedded.h"
#include "MenuBar.h"
#include "MenuBarView.h"
#include "SkiaCanvas.h"
#include "View.h"

#include <gtest/gtest.h>

#include <memory>
#include <optional>
#include <string_view>
#include <vector>

namespace knect4::app
{
namespace
{

/** The window's host with no window: text measured as Skia draws it. */
class TestHost final : public reckless::view::Host
{
  public:
    /** Nothing to repaint. @param root_area The area. */
    void Invalidate(reckless::gfx::RectI root_area) override
    {
        static_cast<void>(root_area);
    }

    /**
     * Text measured as Skia draws it.
     * @param font The font.
     * @param text The text.
     * @return Its width.
     */
    float TextWidth(const reckless::gfx::Font &font, std::string_view text) const override
    {
        return reckless::gfx::MeasureText(font, text);
    }
};

/**
 * The button of a view, or of a view inside it, with this text.
 * @param view The view searched, with every view under it.
 * @param text The button's text.
 * @return The button; null where there is none.
 */
reckless::view::Button *ButtonOf(reckless::view::View &view, std::string_view text)
{
    if (auto *button = dynamic_cast<reckless::view::Button *>(&view); button != nullptr && button->text() == text)
    {
        return button;
    }
    for (reckless::view::View *child : view.children())
    {
        if (auto *found = ButtonOf(*child, text))
        {
            return found;
        }
    }
    return nullptr;
}

/**
 * Every button of a view and the views inside it with this text, in the order they were added.
 * @param view The view searched.
 * @param text The buttons' text.
 * @param found Where they are gathered.
 */
void ButtonsOf(reckless::view::View &view, std::string_view text, std::vector<reckless::view::Button *> &found)
{
    if (auto *button = dynamic_cast<reckless::view::Button *>(&view); button != nullptr && button->text() == text)
    {
        found.push_back(button);
    }
    for (reckless::view::View *child : view.children())
    {
        ButtonsOf(*child, text, found);
    }
}

/**
 * A press and a release in the middle of a button.
 * @param button The button.
 */
void Press(reckless::view::Button &button)
{
    reckless::view::MouseEvent event;
    event.position = {static_cast<float>(button.width()) / 2.0f, static_cast<float>(button.height()) / 2.0f};
    event.down     = event.position;
    button.MouseDown(event);
    button.MouseUp(event);
}

/**
 * A view tree drawn into an image, as a PNG's bytes.
 * @param view The view, laid out.
 * @return The PNG; empty where nothing could be drawn.
 */
std::vector<std::uint8_t> Painted(reckless::view::View &view)
{
    reckless::gfx::Image image = reckless::gfx::MakeImage(view.width(), view.height());
    {
        const auto canvas = reckless::gfx::MakeImageCanvas(image, 1.0f);
        if (!canvas)
        {
            return {};
        }
        reckless::view::PaintTree(view, *canvas);
    }
    return reckless::gfx::EncodePng(image);
}

/** The serif handed to Skia once, as the app does, so text lays out as it draws. */
class Views : public ::testing::Test
{
  protected:
    static void SetUpTestSuite()
    {
        reckless::gfx::SetSerifFaces(reckless::embedded::Find("NotoSerif-Regular.ttf"),
                                     reckless::embedded::Find("NotoSerif-Bold.ttf"));
    }

    TestHost host_; ///< The views' host.
};

/** A mouse event at a point. @param x Its x. @param y Its y. @param right Whether the secondary button. @return It. */
reckless::view::MouseEvent At(float x, float y, bool right = false)
{
    reckless::view::MouseEvent event;
    event.position = {x, y};
    event.down     = event.position;
    event.right    = right;
    return event;
}

/** A key. @param key Which. @param character Its character, for kCharacter. @return The event. */
reckless::view::KeyEvent KeyOf(reckless::view::Key key, char32_t character = 0)
{
    reckless::view::KeyEvent event;
    event.key       = key;
    event.character = character;
    return event;
}

/** A board view laid out at exactly its design size and the room round it, so a view point is a board point + 12. */
class Board : public Views
{
  protected:
    void SetUp() override
    {
        board_.SetHost(&host_);
        board_.SetBounds({0, 0, 784, 728});
        board_.on_hover      = [this](int column) { hovered_.push_back(column); };
        board_.on_drop       = [this](int column) { dropped_.push_back(column); };
        board_.on_step       = [this](int delta) { steps_.push_back(delta); };
        board_.on_drop_held  = [this] { ++drops_held_; };
        board_.on_play_again = [this] { ++plays_again_; };
    }

    /** @param column A column. @return The view's x over it. */
    static float XOver(int column)
    {
        return art::ColumnX(static_cast<float>(column)) + 12.0f;
    }

    BoardView        board_;          ///< The board.
    std::vector<int> hovered_;        ///< The columns told.
    std::vector<int> dropped_;        ///< The drops told.
    std::vector<int> steps_;          ///< The steps told.
    int              drops_held_{0};  ///< The held puck's drops told.
    int              plays_again_{0}; ///< "Play again"s told.
};

TEST_F(Board, MapsTheViewOntoTheBoardAtItsScale)
{
    const reckless::gfx::PointF point = board_.ToBoard({112.0f, 62.0f});
    EXPECT_NEAR(point.x, 100.0f, 0.01f);
    EXPECT_NEAR(point.y, 50.0f, 0.01f);
    // Twice the size: the board twice as large, centred.
    board_.SetBounds({0, 0, 1568, 1456});
    const reckless::gfx::PointF doubled = board_.ToBoard({224.0f, 124.0f});
    EXPECT_NEAR(doubled.x, 100.0f, 0.01f);
    EXPECT_NEAR(doubled.y, 50.0f, 0.01f);
}

TEST_F(Board, TellsTheColumnUnderThePointerAndAClickOnIt)
{
    BoardPicture picture;
    picture.may_choose = true;
    board_.SetPicture(picture);
    board_.MouseMove(At(XOver(2), 300.0f));
    board_.MouseMove(At(XOver(2) + 10.0f, 320.0f)); // the same column: told once
    board_.MouseDrag(At(XOver(5), 300.0f));
    // Every move over the board is told, so the held puck comes back under the pointer after the keys moved it.
    EXPECT_EQ(hovered_, (std::vector<int>{2, 2, 5}));
    EXPECT_EQ(board_.cursor, reckless::view::Cursor::kPointingHand);

    board_.MouseDown(At(XOver(5), 300.0f));
    board_.MouseDown(At(XOver(5), 300.0f, true)); // a right press drops nothing
    EXPECT_EQ(dropped_, (std::vector<int>{5}));

    // Off the frame's sides, below the board, and the pointer leaving: no column.
    board_.MouseMove(At(20.0f, 300.0f));
    board_.MouseMove(At(XOver(3), 720.0f));
    board_.MouseExit(At(0.0f, 0.0f));
    EXPECT_EQ(hovered_.back(), -1);
    board_.MouseDown(At(20.0f, 300.0f));
    EXPECT_EQ(dropped_.size(), 1u);
    EXPECT_EQ(board_.cursor, reckless::view::Cursor::kArrow);
}

TEST_F(Board, OffersPlayAgainOnTheBannerOnly)
{
    const reckless::gfx::RectF area = BoardView::PlayAgainArea();
    const float                x    = area.CentreX() + 12.0f;
    const float                y    = area.CentreY() + 12.0f;
    board_.MouseDown(At(x, y)); // no banner: the press is over the board's top, a column
    EXPECT_EQ(plays_again_, 0);

    BoardPicture over;
    over.banner = "You win!";
    board_.SetPicture(over);
    board_.MouseMove(At(x, y));
    EXPECT_EQ(board_.cursor, reckless::view::Cursor::kPointingHand);
    board_.SetPicture(over); // the pointer still over it keeps the hand
    EXPECT_EQ(board_.cursor, reckless::view::Cursor::kPointingHand);
    board_.MouseDown(At(x, y));
    EXPECT_EQ(plays_again_, 1);
    // Return plays again while the banner shows.
    EXPECT_TRUE(board_.KeyPressed(KeyOf(reckless::view::Key::kReturn)));
    EXPECT_EQ(plays_again_, 2);
    EXPECT_EQ(drops_held_, 0);
}

TEST_F(Board, TakesTheKeysThatPlay)
{
    using reckless::view::Key;
    EXPECT_TRUE(board_.WantsKeyboardFocus());
    EXPECT_TRUE(board_.KeepsKeyboardFocus()); // a press on the panel's plates leaves the keys with the board
    EXPECT_TRUE(board_.KeyPressed(KeyOf(Key::kLeft)));
    EXPECT_TRUE(board_.KeyPressed(KeyOf(Key::kRight)));
    EXPECT_EQ(steps_, (std::vector<int>{-1, 1}));
    EXPECT_TRUE(board_.KeyPressed(KeyOf(Key::kReturn)));
    EXPECT_TRUE(board_.KeyPressed(KeyOf(Key::kDown)));
    EXPECT_TRUE(board_.KeyPressed(KeyOf(Key::kCharacter, U' ')));
    EXPECT_EQ(drops_held_, 3);
    EXPECT_TRUE(board_.KeyPressed(KeyOf(Key::kCharacter, U'1')));
    EXPECT_TRUE(board_.KeyPressed(KeyOf(Key::kCharacter, U'7')));
    EXPECT_EQ(dropped_, (std::vector<int>{0, 6}));
    // Not the game's: another character, an unhandled key, anything with a modifier the menus' shortcuts take.
    EXPECT_FALSE(board_.KeyPressed(KeyOf(Key::kCharacter, U'8')));
    EXPECT_FALSE(board_.KeyPressed(KeyOf(Key::kCharacter, U'a')));
    EXPECT_FALSE(board_.KeyPressed(KeyOf(Key::kEscape)));
    reckless::view::KeyEvent command = KeyOf(Key::kCharacter, U'1');
    command.modifiers.command        = true;
    EXPECT_FALSE(board_.KeyPressed(command));
    reckless::view::KeyEvent alt = KeyOf(Key::kLeft);
    alt.modifiers.alt            = true;
    EXPECT_FALSE(board_.KeyPressed(alt));
    EXPECT_EQ(steps_.size(), 2u);
}

TEST_F(Board, ListensToNoOneWhenNoOneListens)
{
    BoardView quiet;
    quiet.SetHost(&host_);
    quiet.SetBounds({0, 0, 784, 728});
    BoardPicture over;
    over.banner = "A draw";
    quiet.SetPicture(over);
    using reckless::view::Key;
    quiet.MouseMove(At(XOver(1), 300.0f));
    quiet.MouseDown(At(XOver(1), 300.0f));
    const reckless::gfx::RectF area = BoardView::PlayAgainArea();
    quiet.MouseDown(At(area.CentreX() + 12.0f, area.CentreY() + 12.0f));
    quiet.MouseExit(At(0.0f, 0.0f));
    for (const auto key : {KeyOf(Key::kLeft), KeyOf(Key::kReturn), KeyOf(Key::kDown), KeyOf(Key::kCharacter, U'3'),
                           KeyOf(Key::kCharacter, U' ')})
    {
        EXPECT_TRUE(quiet.KeyPressed(key));
    }
    EXPECT_EQ(quiet.picture().banner, "A draw");
}

TEST_F(Views, PaintsEveryStateOfTheWindow)
{
    MainView view;
    view.SetHost(&host_);
    view.SetBounds({0, 0, MainView::kDesignWidth, MainView::kDesignHeight});

    std::vector<std::vector<std::uint8_t>> pictures;
    const auto                             paint = [&](const Controller &controller) {
        view.Show(controller);
        pictures.push_back(Painted(view));
        ASSERT_FALSE(pictures.back().empty());
    };
    // Against the computer: the person's puck held, a puck falling, a win and a loss; then between two people.
    Controller computer(nullptr);
    computer.PlaceMoves({3, 3, 2, 4});
    computer.Hover(5);
    computer.Frame(0.25);
    paint(computer);
    computer.Drop(5);
    computer.Frame(0.1);
    paint(computer);
    computer.PlaceMoves({0, 0, 1, 1, 2, 2, 3});
    paint(computer);
    computer.PlaceMoves({0, 3, 0, 3, 1, 3, 6, 3});
    paint(computer);
    Settings two;
    two.is_two_player = true;
    Controller friends(nullptr, two);
    friends.PlaceMoves({2, 1, 3, 2, 4, 4, 3, 3, 4, 5});
    friends.Hover(1);
    friends.Frame(0.25);
    paint(friends);
    friends.PlaceMoves({0, 6, 0, 6, 1, 6, 2, 6});
    paint(friends);
    // Each state draws something of its own.
    for (std::size_t index = 1; index < pictures.size(); ++index)
    {
        EXPECT_NE(pictures[index], pictures[index - 1]) << "picture " << index;
    }
}

TEST_F(Views, PanelTellsEveryChoice)
{
    SidePanel panel;
    panel.SetHost(&host_);
    panel.SetBounds({0, 0, SidePanel::kWidth, MainView::kDesignHeight});
    std::optional<bool>  opponent;
    std::optional<Level> level;
    std::optional<bool>  colour;
    std::optional<bool>  starter;
    std::optional<bool>  sound;
    int                  new_games = 0;
    panel.on_opponent              = [&](bool is_two_player) { opponent = is_two_player; };
    panel.on_level                 = [&](Level chosen) { level = chosen; };
    panel.on_colour                = [&](bool is_red) { colour = is_red; };
    panel.on_starter               = [&](bool does_person_start) { starter = does_person_start; };
    panel.on_sound                 = [&](bool has_sound) { sound = has_sound; };
    panel.on_new_game              = [&] { ++new_games; };

    Controller controller(nullptr);
    panel.Show(controller);
    EXPECT_EQ(panel.headline(), "Your move");
    for (const char *text : {"A friend", "Hard", "Yellow", "New game", "Sound"})
    {
        reckless::view::Button *button = ButtonOf(panel, text);
        ASSERT_NE(button, nullptr) << text;
        Press(*button);
    }
    // "Computer" names both the opponent and who starts: the opponent's plate is the first.
    std::vector<reckless::view::Button *> computers;
    ButtonsOf(panel, "Computer", computers);
    ASSERT_EQ(computers.size(), 2u);
    Press(*computers[1]);
    EXPECT_EQ(opponent, true);
    EXPECT_EQ(level, Level::kHard);
    EXPECT_EQ(colour, false);
    EXPECT_EQ(starter, false);
    EXPECT_EQ(new_games, 1);
    EXPECT_EQ(sound, false); // the toggle was on, as the settings have it, and the press turned it off
    Press(*ButtonOf(panel, "Easy"));
    Press(*ButtonOf(panel, "Red"));
    Press(*ButtonOf(panel, "You"));
    EXPECT_EQ(level, Level::kEasy);
    EXPECT_EQ(colour, true);
    EXPECT_EQ(starter, true);
    Press(*computers[0]);
    EXPECT_EQ(opponent, false);

    // Between two people: the level and the colour dimmed, the first move named by colour, the score by colour.
    controller.SetTwoPlayer(true);
    panel.Show(controller);
    EXPECT_EQ(panel.headline(), "Red to move");
    EXPECT_TRUE(ButtonOf(panel, "Medium")->dimmed());
    EXPECT_TRUE(ButtonOf(panel, "Red") != nullptr);
    EXPECT_EQ(ButtonOf(panel, "You"), nullptr);
    EXPECT_FALSE(Painted(panel).empty());
}

TEST_F(Views, PanelPaintsTheWinnersColours)
{
    SidePanel panel;
    panel.SetHost(&host_);
    panel.SetBounds({0, 0, SidePanel::kWidth, MainView::kDesignHeight});
    Controller controller(nullptr);
    controller.PlaceMoves({0, 0, 1, 1, 2, 2, 3});
    panel.Show(controller);
    EXPECT_EQ(panel.headline(), "You win!");
    const std::vector<std::uint8_t> won = Painted(panel);
    controller.PlaceMoves({0, 3, 0, 3, 1, 3, 6, 3});
    panel.Show(controller);
    EXPECT_EQ(panel.headline(), "I win");
    EXPECT_NE(Painted(panel), won);
    Settings two;
    two.is_two_player = true;
    Controller friends(nullptr, two);
    friends.PlaceMoves({0, 6, 0, 6, 1, 6, 2, 6});
    panel.Show(friends);
    EXPECT_EQ(panel.headline(), "Yellow wins!");
    panel.Show(friends); // the same again: nothing to repaint
    EXPECT_FALSE(Painted(panel).empty());
}

TEST_F(Views, LaysTheDrawnMenuBarAcrossTheTop)
{
    MainView view;
    view.SetHost(&host_);
    view.SetBounds({0, 0, MainView::kDesignWidth, MainView::kDesignHeight});
    EXPECT_EQ(view.board().bounds().y(), 0);
    auto controller = std::make_shared<reckless::app::MenuBarController>();
    controller->Set(reckless::app::MenuBar{{reckless::app::MenuEntry::Menu(
                        "Game", {reckless::app::MenuEntry::Item("New game", 1)})}},
                    {});
    view.SetMenuBarView(std::make_unique<reckless::app::MenuBarView>(controller, theme::MenuBarLook()));
    const int bar = theme::MenuBarLook().height;
    EXPECT_EQ(view.board().bounds().y(), bar);
    EXPECT_EQ(view.panel().bounds().y(), bar);
    EXPECT_EQ(view.board().height(), MainView::kDesignHeight - bar);
    EXPECT_FALSE(Painted(view).empty());
    // Another bar takes its place; none puts the board back at the top.
    view.SetMenuBarView(std::make_unique<reckless::app::MenuBarView>(controller, theme::MenuBarLook()));
    view.SetMenuBarView(nullptr);
    EXPECT_EQ(view.board().bounds().y(), 0);
}

} // namespace
} // namespace knect4::app
