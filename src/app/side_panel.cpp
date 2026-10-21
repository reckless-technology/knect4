#include "app/side_panel.h"

#include "app/board_art.h"
#include "app/theme.h"

#include <string_view>
#include <utility>

namespace knect4::app
{

namespace
{

using reckless::gfx::RectF;
using reckless::gfx::RectI;

/**
 * A plate for a colour: the window's plate with a small puck of that colour at its left.
 * @param colours The puck's colours.
 * @return The painter.
 */
reckless::view::Button::Painter ColourPlatePainter(theme::PuckColours colours)
{
    return [colours](reckless::gfx::Canvas &c, RectI bounds, std::string_view text, reckless::view::Button::Look look) {
        theme::DrawChoicePlate(c, bounds, look);
        const float radius = static_cast<float>(bounds.height()) * 0.3f;
        if (look.dimmed)
        {
            c.BeginTransparencyLayer(0.4f);
        }
        art::DrawPuck(c, {14.0f + radius, static_cast<float>(bounds.height()) / 2.0f}, radius, colours);
        if (look.dimmed)
        {
            c.EndTransparencyLayer();
        }
        c.SetFont(fonts::Plate());
        c.SetColor(theme::ChoiceInk(look));
        const float left = 20.0f + 2.0f * radius;
        c.DrawText(
            text,
            RectF{left, 0.0f, static_cast<float>(bounds.width()) - left - 6.0f, static_cast<float>(bounds.height())},
            reckless::gfx::Justify::kCentredLeft, true);
    };
}

constexpr int kMargin      = 18; ///< The panel's margin.
constexpr int kPlateHeight = 36; ///< A plate's height.
constexpr int kGap         = 8;  ///< Between plates.

} // namespace

ScoreView::ScoreView()
{
    SetInterceptsMouse(false, false);
}

void ScoreView::SetScore(const Score &score, bool is_two_player)
{
    if (score.min_wins != score_.min_wins || score.max_wins != score_.max_wins || score.drawn != score_.drawn ||
        is_two_player != is_two_player_)
    {
        score_         = score;
        is_two_player_ = is_two_player;
        Repaint();
    }
}

void ScoreView::Paint(reckless::gfx::Canvas &c)
{
    // Against the computer the person's wins in the good colour and the computer's in the bad; between two people each
    // side's in its own pucks' colour.
    const std::array<std::pair<int, const char *>, 3> counts{{{score_.min_wins, is_two_player_ ? "Red" : "You"},
                                                              {score_.max_wins, is_two_player_ ? "Yellow" : "Computer"},
                                                              {score_.drawn, "Drawn"}}};
    const std::array<reckless::gfx::Color, 3> colours{is_two_player_ ? theme::kRed.light : theme::kGood,
                                                      is_two_player_ ? theme::kYellow.body : theme::kBad, theme::kInk};
    const float                               column_width = static_cast<float>(width()) / 3.0f;
    for (std::size_t index = 0; index < counts.size(); ++index)
    {
        const float left = column_width * static_cast<float>(index);
        c.SetFont(fonts::Count());
        c.SetColor(colours[index]);
        c.DrawText(std::to_string(counts[index].first), RectF{left, 0.0f, column_width, 42.0f},
                   reckless::gfx::Justify::kCentred, false);
        c.SetFont(fonts::Small());
        c.SetColor(theme::kMuted);
        c.DrawText(counts[index].second, RectF{left, 42.0f, column_width, 20.0f}, reckless::gfx::Justify::kCentred,
                   false);
    }
}

SidePanel::SidePanel()
{
    SetOpaque(true);
    const reckless::app::Theme shell = theme::Shell();
    title_                           = std::make_unique<reckless::app::TextLine>(theme::kAmber, fonts::Label());
    headline_                        = std::make_unique<reckless::app::TextLine>(theme::kInk, fonts::Headline());
    detail_                          = std::make_unique<reckless::app::TextLine>(theme::kMuted, fonts::Small());
    title_->SetText("KNECT 4");
    for (auto *line : {title_.get(), headline_.get(), detail_.get()})
    {
        AddChild(*line);
    }
    opponent_section_ = std::make_unique<reckless::app::Section>(shell, "Play against");
    level_section_    = std::make_unique<reckless::app::Section>(shell, "Level");
    colour_section_   = std::make_unique<reckless::app::Section>(shell, "Your pucks");
    starter_section_  = std::make_unique<reckless::app::Section>(shell, "First move");
    score_section_    = std::make_unique<reckless::app::Section>(shell, "Score");
    for (auto *section : {opponent_section_.get(), level_section_.get(), colour_section_.get(), starter_section_.get(),
                          score_section_.get()})
    {
        AddChild(*section);
    }

    for (const bool is_two_player : {false, true})
    {
        opponents_[is_two_player ? 1 : 0] = AddChoice(is_two_player ? "A friend" : "Computer", [this, is_two_player] {
            if (on_opponent)
            {
                on_opponent(is_two_player);
            }
        });
    }

    const std::array<std::pair<const char *, Level>, 3> levels{
        {{"Easy", Level::kEasy}, {"Medium", Level::kMedium}, {"Hard", Level::kHard}}};
    for (std::size_t index = 0; index < levels.size(); ++index)
    {
        const Level level = levels[index].second;
        levels_[index]    = AddChoice(levels[index].first, [this, level] {
            if (on_level)
            {
                on_level(level);
            }
        });
    }
    for (const bool is_red : {true, false})
    {
        auto plate = std::make_unique<reckless::view::Button>(
            is_red ? "Red" : "Yellow", ColourPlatePainter(is_red ? theme::kRed : theme::kYellow), [this, is_red] {
                if (on_colour)
                {
                    on_colour(is_red);
                }
            });
        AddChild(*plate);
        colours_[is_red ? 0 : 1] = std::move(plate);
    }
    for (const bool is_person : {true, false})
    {
        starters_[is_person ? 0 : 1] = AddChoice(is_person ? "You" : "Computer", [this, is_person] {
            if (on_starter)
            {
                on_starter(is_person);
            }
        });
    }
    new_game_ = AddPlate("New game", [this] {
        if (on_new_game)
        {
            on_new_game();
        }
    });
    sound_    = std::make_unique<reckless::view::Button>(
        "Sound", theme::PlatePainter(),
        [this] {
            if (on_sound)
            {
                on_sound(sound_->on());
            }
        },
        true);
    sound_->SetShowsLamp(true);
    AddChild(*sound_);
    score_ = std::make_unique<ScoreView>();
    AddChild(*score_);
}

std::unique_ptr<reckless::view::Button> SidePanel::AddPlate(std::string text, std::function<void()> clicked)
{
    auto plate = std::make_unique<reckless::view::Button>(std::move(text), theme::PlatePainter(), std::move(clicked));
    AddChild(*plate);
    return plate;
}

std::unique_ptr<reckless::view::Button> SidePanel::AddChoice(std::string text, std::function<void()> clicked)
{
    auto plate = std::make_unique<reckless::view::Button>(std::move(text), theme::ChoicePainter(), std::move(clicked));
    AddChild(*plate);
    return plate;
}

void SidePanel::Show(const Controller &controller)
{
    const Settings &settings = controller.settings();
    const bool      is_two   = settings.is_two_player;
    headline_->SetText(controller.StatusText());
    reckless::gfx::Color headline_colour = theme::kInk;
    if (controller.phase() == Phase::kWon)
    {
        headline_colour = is_two ? controller.ColoursOf(controller.winner()).light
                                 : (controller.winner() == kMin ? theme::kGood : theme::kBad);
    }
    headline_->SetColour(headline_colour);
    static constexpr std::array<const char *, 3> kLevelNames{"Easy", "Medium", "Hard"};
    detail_->SetText(is_two ? std::string("Two players  ·  Red and Yellow take turns")
                            : std::string(settings.is_person_red ? "You play red" : "You play yellow") + "  ·  " +
                                  kLevelNames[static_cast<std::size_t>(settings.level)]);
    opponents_[0]->SetOn(!is_two);
    opponents_[1]->SetOn(is_two);
    // Between two people there is no computer to set a level for, and the colours are the players' names.
    for (std::size_t index = 0; index < levels_.size(); ++index)
    {
        levels_[index]->SetOn(static_cast<std::size_t>(settings.level) == index);
        levels_[index]->SetDimmed(is_two);
    }
    colours_[0]->SetOn(settings.is_person_red);
    colours_[1]->SetOn(!settings.is_person_red);
    colours_[0]->SetDimmed(is_two);
    colours_[1]->SetDimmed(is_two);
    starters_[0]->SetText(is_two ? "Red" : "You");
    starters_[1]->SetText(is_two ? "Yellow" : "Computer");
    starters_[0]->SetOn(settings.does_person_start);
    starters_[1]->SetOn(!settings.does_person_start);
    sound_->SetOn(settings.has_sound);
    score_->SetScore(controller.score(), is_two);
}

void SidePanel::Paint(reckless::gfx::Canvas &c)
{
    c.FillAll(theme::kPanel);
    c.SetColor(theme::kRule);
    c.DrawVerticalLine(0, 0.0f, static_cast<float>(height()));
}

void SidePanel::Resized()
{
    const int inner = width() - 2 * kMargin;
    int       y     = 20;
    title_->SetBounds({kMargin, y, inner, 20});
    y += 24;
    headline_->SetBounds({kMargin, y, inner, 42});
    y += 42;
    detail_->SetBounds({kMargin, y, inner, 22});
    y += 36;

    // A section holding one row of plates, the row split evenly.
    const auto lay_row = [&](reckless::app::Section &section, auto &plates) {
        const int section_height =
            reckless::app::Section::kTitleHeight + kPlateHeight + 2 * reckless::app::Section::kPadding;
        section.SetBounds({kMargin, y, inner, section_height});
        const RectI area  = section.ContentArea().Translated(section.bounds().x(), section.bounds().y());
        const int   count = static_cast<int>(plates.size());
        const int   each  = (area.width() - (count - 1) * kGap) / count;
        for (int index = 0; index < count; ++index)
        {
            plates[static_cast<std::size_t>(index)]->SetBounds(
                {area.x() + index * (each + kGap), area.y() + (area.height() - kPlateHeight) / 2, each, kPlateHeight});
        }
        y += section_height + 12;
    };
    lay_row(*opponent_section_, opponents_);
    lay_row(*level_section_, levels_);
    lay_row(*colour_section_, colours_);
    lay_row(*starter_section_, starters_);

    new_game_->SetBounds({kMargin, y, inner, 44});
    y += 44 + 18;

    const int score_height = reckless::app::Section::kTitleHeight + 64 + 2 * reckless::app::Section::kPadding;
    score_section_->SetBounds({kMargin, y, inner, score_height});
    const RectI score_area = score_section_->ContentArea().Translated(kMargin, y);
    score_->SetBounds({score_area.x(), score_area.y(), score_area.width(), 64});
    y += score_height + 12;

    sound_->SetBounds({kMargin, std::max(y, height() - kMargin - kPlateHeight), 120, kPlateHeight});
}

} // namespace knect4::app
