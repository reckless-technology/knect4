#pragma once
/// @file side_panel.h The panel beside the board: the game's state, the person's choices - the opponent, the level, the
/// colour or the players' names, who starts, the board's size, the sound and the marks - a new game, undo, a hint, and
/// the score.

#include "controller.h"

#include "AppWidgets.h"
#include "Button.h"
#include "TextField.h"
#include "View.h"

#include <array>
#include <functional>
#include <memory>
#include <string>

namespace knect4::app
{

/** The score: the games each side won and the games drawn, as three large counts. */
class ScoreView final : public reckless::view::View
{
  public:
    ScoreView();

    /**
     * The counts shown; repaints when they differ.
     * @param score The games so far.
     * @param first The first count's label: "You", or Red's name.
     * @param second The second's: "Computer", or Yellow's name.
     * @param is_two_player Whether they are games between two people, counted in the players' colours.
     */
    void SetScore(const Score &score, std::string first, std::string second, bool is_two_player);

    void Paint(reckless::gfx::Canvas &c) override;

  private:
    Score       score_;                ///< The counts shown.
    std::string first_{"You"};         ///< The first count's label.
    std::string second_{"Computer"};   ///< The second's.
    bool        is_two_player_{false}; ///< They are Red's and Yellow's.
};

/**
 * The panel: a headline of where the game stands, then sections of plates for the choices - each group lit on its
 * choice - "New game", "Undo" and "Hint", the score, and the sound's and the marks' switches. Against the computer it
 * shows the level and the colour; between two people, the players' names in their place. Show() sets it from the
 * controller each frame; the `on_*` callbacks tell what the person chose.
 */
class SidePanel final : public reckless::view::View
{
  public:
    static constexpr int kWidth = 320; ///< The panel across, in design pixels.

    SidePanel();

    std::function<void(bool is_two_player)>                  on_opponent;  ///< The opponent chosen.
    std::function<void(Level level)>                         on_level;     ///< A level chosen.
    std::function<void(bool is_person_red)>                  on_colour;    ///< A colour chosen.
    std::function<void(bool does_person_start)>              on_starter;   ///< Who starts, chosen.
    std::function<void(bool does_alternate)>                 on_alternate; ///< Taking turns to start, switched.
    std::function<void(BoardSize size)>                      on_board;     ///< A board size chosen.
    std::function<void(bool has_sound)>                      on_sound;     ///< The sound switched.
    std::function<void(bool has_marks)>                      on_marks;     ///< The marks switched.
    std::function<void()>                                    on_new_game;  ///< "New game".
    std::function<void()>                                    on_undo;      ///< "Undo".
    std::function<void()>                                    on_hint;      ///< "Hint".
    std::function<void(std::string red, std::string yellow)> on_names;     ///< A player's name entered.

    /**
     * The panel as the controller stands.
     * @param controller The game.
     */
    void Show(const Controller &controller);

    /** @return The headline shown. */
    const std::string &headline() const noexcept
    {
        return headline_->text();
    }

    /** @return Red's name field. */
    reckless::view::TextField &red_name() noexcept
    {
        return *red_name_;
    }

    /** @return Yellow's name field. */
    reckless::view::TextField &yellow_name() noexcept
    {
        return *yellow_name_;
    }

    void Paint(reckless::gfx::Canvas &c) override;
    void PaintOverChildren(reckless::gfx::Canvas &c) override;
    void Resized() override;

  private:
    /**
     * A plate that does something.
     * @param text Its caption.
     * @param clicked What it does.
     * @return The plate, added to the panel.
     */
    std::unique_ptr<reckless::view::Button> AddPlate(std::string text, std::function<void()> clicked);

    /**
     * A plate in a group of choices, amber while it is the one taken.
     * @param text Its caption.
     * @param clicked What it does.
     * @param is_compact Whether its caption is set smaller, for a row of four or five.
     * @return The plate, added to the panel.
     */
    std::unique_ptr<reckless::view::Button> AddChoice(std::string text, std::function<void()> clicked,
                                                      bool is_compact = false);

    /** The names entered, told. */
    void TellNames();

    bool is_two_player_{false}; ///< The layout shows two people's sections.

    std::unique_ptr<reckless::app::TextLine>               title_;            ///< The game's name.
    std::unique_ptr<reckless::app::TextLine>               headline_;         ///< Where the game stands.
    std::unique_ptr<reckless::app::TextLine>               detail_;           ///< Who plays what, at what level.
    std::unique_ptr<reckless::app::Section>                opponent_section_; ///< "Play against".
    std::unique_ptr<reckless::app::Section>                level_section_;    ///< "Level".
    std::unique_ptr<reckless::app::Section>                colour_section_;   ///< "Your pucks".
    std::unique_ptr<reckless::app::Section>                names_section_;    ///< "Players", between two people.
    std::unique_ptr<reckless::app::Section>                starter_section_;  ///< "First move".
    std::unique_ptr<reckless::app::Section>                board_section_;    ///< "Board".
    std::unique_ptr<reckless::app::Section>                score_section_;    ///< "Score".
    std::array<std::unique_ptr<reckless::view::Button>, 2> opponents_;        ///< Computer, A friend.
    std::array<std::unique_ptr<reckless::view::Button>, 4> levels_;           ///< Easy, Medium, Hard, Perfect.
    std::array<std::unique_ptr<reckless::view::Button>, 2> colours_;          ///< Red, Yellow.
    /** You and Computer; between two people Red, Yellow and Take turns. */
    std::array<std::unique_ptr<reckless::view::Button>, 3>                  starters_;
    std::array<std::unique_ptr<reckless::view::Button>, kBoardSizes.size()> boards_;      ///< The board sizes.
    std::unique_ptr<reckless::view::TextField>                              red_name_;    ///< Red's name.
    std::unique_ptr<reckless::view::TextField>                              yellow_name_; ///< Yellow's name.
    std::unique_ptr<reckless::view::Button>                                 new_game_;    ///< "New game".
    std::unique_ptr<reckless::view::Button>                                 undo_;        ///< "Undo".
    std::unique_ptr<reckless::view::Button>                                 hint_;        ///< "Hint".
    std::unique_ptr<reckless::view::Button>                                 sound_;       ///< "Sound", a toggle.
    std::unique_ptr<reckless::view::Button>                                 marks_;       ///< "Marks", a toggle.
    std::unique_ptr<ScoreView>                                              score_;       ///< The counts.
};

} // namespace knect4::app
