#pragma once
/// @file side_panel.h The panel beside the board: the game's state, the person's choices - level, colour, who starts,
/// sound - a new game, and the score.

#include "controller.h"

#include "AppWidgets.h"
#include "Button.h"
#include "View.h"

#include <array>
#include <functional>
#include <memory>
#include <string>

namespace connect4::app
{

/** The score: the games each side won and the games drawn, as three large counts. */
class ScoreView final : public reckless::view::View
{
  public:
    ScoreView();

    /** @param score The games so far; repaints when they differ. */
    void SetScore(const Score &score);

    void Paint(reckless::gfx::Canvas &c) override;

  private:
    Score score_; ///< The counts shown.
};

/**
 * The panel: a headline of where the game stands, then sections of plates for the choices - each group lit on its
 * choice - "New game", the score and the sound's switch. Show() sets it from the controller each frame; the `on_*`
 * callbacks tell what the person chose.
 */
class SidePanel final : public reckless::view::View
{
  public:
    static constexpr int kWidth = 320; ///< The panel across, in design pixels.

    SidePanel();

    std::function<void(Level level)>            on_level;    ///< A level chosen.
    std::function<void(bool is_person_red)>     on_colour;   ///< A colour chosen.
    std::function<void(bool does_person_start)> on_starter;  ///< Who starts, chosen.
    std::function<void(bool has_sound)>         on_sound;    ///< The sound switched.
    std::function<void()>                       on_new_game; ///< "New game".

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

    void Paint(reckless::gfx::Canvas &c) override;
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
     * @return The plate, added to the panel.
     */
    std::unique_ptr<reckless::view::Button> AddChoice(std::string text, std::function<void()> clicked);

    std::unique_ptr<reckless::app::TextLine>               title_;           ///< The game's name.
    std::unique_ptr<reckless::app::TextLine>               headline_;        ///< Where the game stands.
    std::unique_ptr<reckless::app::TextLine>               detail_;          ///< Who plays which colour, at what level.
    std::unique_ptr<reckless::app::Section>                level_section_;   ///< "Level".
    std::unique_ptr<reckless::app::Section>                colour_section_;  ///< "Your pucks".
    std::unique_ptr<reckless::app::Section>                starter_section_; ///< "First move".
    std::unique_ptr<reckless::app::Section>                score_section_;   ///< "Score".
    std::array<std::unique_ptr<reckless::view::Button>, 3> levels_;          ///< Easy, Medium, Hard.
    std::array<std::unique_ptr<reckless::view::Button>, 2> colours_;         ///< Red, Yellow.
    std::array<std::unique_ptr<reckless::view::Button>, 2> starters_;        ///< You, Computer.
    std::unique_ptr<reckless::view::Button>                new_game_;        ///< "New game".
    std::unique_ptr<reckless::view::Button>                sound_;           ///< "Sound", a toggle.
    std::unique_ptr<ScoreView>                             score_;           ///< The counts.
};

} // namespace connect4::app
