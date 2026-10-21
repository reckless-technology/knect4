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

/**
 * A choice in a row of four or five: the choice plate, its caption a size smaller.
 * @return The painter.
 */
reckless::view::Button::Painter CompactChoicePainter()
{
    return [](reckless::gfx::Canvas &c, RectI bounds, std::string_view text, reckless::view::Button::Look look) {
        theme::DrawChoicePlate(c, bounds, look);
        c.SetFont(fonts::Plate().WithHeight(14.5f));
        c.SetColor(theme::ChoiceInk(look));
        c.DrawText(text,
                   RectF{2.0f, 0.0f, static_cast<float>(bounds.width()) - 4.0f, static_cast<float>(bounds.height())},
                   reckless::gfx::Justify::kCentred, true);
    };
}

/** @return The name fields' look: the window's colours, amber where the keyboard is. */
reckless::view::TextField::Style FieldStyle()
{
    reckless::view::TextField::Style style;
    style.font             = fonts::Small().WithHeight(17.0f);
    style.text             = theme::kInk;
    style.background       = theme::kWindow.Darker(0.3f);
    style.outline          = theme::kRule;
    style.focused_outline  = theme::kAmber;
    style.highlight        = theme::kSelect;
    style.highlighted_text = theme::kInk;
    style.caret            = theme::kAmber;
    style.justify          = reckless::gfx::Justify::kCentredLeft;
    style.left_indent      = 8;
    style.max_length       = 24;
    return style;
}

/**
 * @param size A board's size.
 * @return Its plate's caption: "7×6".
 */
std::string SizeCaption(BoardSize size)
{
    return std::to_string(size.columns) + "×" + std::to_string(size.rows);
}

constexpr int kMargin      = 18; ///< The panel's margin.
constexpr int kPlateHeight = 36; ///< A plate's height.
constexpr int kGap         = 6;  ///< Between plates.
constexpr int kFieldHeight = 30; ///< A name field's height.

} // namespace

ScoreView::ScoreView()
{
    SetInterceptsMouse(false, false);
}

void ScoreView::SetScore(const Score &score, std::string first, std::string second, bool is_two_player)
{
    if (!(score == score_) || first != first_ || second != second_ || is_two_player != is_two_player_)
    {
        score_         = score;
        first_         = std::move(first);
        second_        = std::move(second);
        is_two_player_ = is_two_player;
        Repaint();
    }
}

void ScoreView::Paint(reckless::gfx::Canvas &c)
{
    // Against the computer the person's wins in the good colour and the computer's in the bad; between two people each
    // side's in its own pucks' colour.
    const std::array<std::pair<int, std::string>, 3> counts{
        {{score_.min_wins, first_}, {score_.max_wins, second_}, {score_.drawn, "Drawn"}}};
    const std::array<reckless::gfx::Color, 3> colours{is_two_player_ ? theme::kRed.light : theme::kGood,
                                                      is_two_player_ ? theme::kYellow.body : theme::kBad, theme::kInk};
    const float                               column_width = static_cast<float>(width()) / 3.0f;
    for (std::size_t index = 0; index < counts.size(); ++index)
    {
        const float left = column_width * static_cast<float>(index);
        c.SetFont(fonts::Count());
        c.SetColor(colours[index]);
        c.DrawText(std::to_string(counts[index].first), RectF{left, 0.0f, column_width, 38.0f},
                   reckless::gfx::Justify::kCentred, false);
        c.SetFont(fonts::Small());
        c.SetColor(theme::kMuted);
        c.DrawText(counts[index].second, RectF{left + 2.0f, 38.0f, column_width - 4.0f, 20.0f},
                   reckless::gfx::Justify::kCentred, true);
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
    names_section_    = std::make_unique<reckless::app::Section>(shell, "Players");
    starter_section_  = std::make_unique<reckless::app::Section>(shell, "First move");
    board_section_    = std::make_unique<reckless::app::Section>(shell, "Board");
    score_section_    = std::make_unique<reckless::app::Section>(shell, "Score");
    for (auto *section : {opponent_section_.get(), level_section_.get(), colour_section_.get(), names_section_.get(),
                          starter_section_.get(), board_section_.get(), score_section_.get()})
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
    const std::array<std::pair<const char *, Level>, 4> levels{
        {{"Easy", Level::kEasy}, {"Medium", Level::kMedium}, {"Hard", Level::kHard}, {"Perfect", Level::kPerfect}}};
    for (std::size_t index = 0; index < levels.size(); ++index)
    {
        const Level level = levels[index].second;
        levels_[index]    = AddChoice(
            levels[index].first,
            [this, level] {
                if (on_level)
                {
                    on_level(level);
                }
            },
            true);
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
    red_name_    = std::make_unique<reckless::view::TextField>(FieldStyle());
    yellow_name_ = std::make_unique<reckless::view::TextField>(FieldStyle());
    for (auto *field : {red_name_.get(), yellow_name_.get()})
    {
        field->on_commit = [this](const std::string &) { TellNames(); };
        AddChild(*field);
    }
    for (const bool is_first : {true, false})
    {
        starters_[is_first ? 0 : 1] = AddChoice(is_first ? "You" : "Computer", [this, is_first] {
            if (on_starter)
            {
                on_starter(is_first);
            }
        });
    }
    starters_[2] = AddChoice("Take turns", [this] {
        if (on_alternate)
        {
            on_alternate(!starters_[2]->on());
        }
    });
    for (std::size_t index = 0; index < kBoardSizes.size(); ++index)
    {
        const BoardSize size = kBoardSizes[index];
        boards_[index]       = AddChoice(
            SizeCaption(size),
            [this, size] {
                if (on_board)
                {
                    on_board(size);
                }
            },
            true);
    }
    new_game_ = AddPlate("New game", [this] {
        if (on_new_game)
        {
            on_new_game();
        }
    });
    undo_     = AddPlate("Undo", [this] {
        if (on_undo)
        {
            on_undo();
        }
    });
    hint_     = AddPlate("Hint", [this] {
        if (on_hint)
        {
            on_hint();
        }
    });
    // The toggles tell their state after the press has flipped it.
    sound_ = std::make_unique<reckless::view::Button>(
        "Sound", theme::PlatePainter(),
        [this] {
            if (on_sound)
            {
                on_sound(sound_->on());
            }
        },
        true);
    marks_ = std::make_unique<reckless::view::Button>(
        "Marks", theme::PlatePainter(),
        [this] {
            if (on_marks)
            {
                on_marks(marks_->on());
            }
        },
        true);
    for (auto *toggle : {sound_.get(), marks_.get()})
    {
        toggle->SetShowsLamp(true);
        AddChild(*toggle);
    }
    score_ = std::make_unique<ScoreView>();
    AddChild(*score_);
}

std::unique_ptr<reckless::view::Button> SidePanel::AddPlate(std::string text, std::function<void()> clicked)
{
    auto plate = std::make_unique<reckless::view::Button>(std::move(text), theme::PlatePainter(), std::move(clicked));
    AddChild(*plate);
    return plate;
}

std::unique_ptr<reckless::view::Button> SidePanel::AddChoice(std::string text, std::function<void()> clicked,
                                                             bool is_compact)
{
    auto plate = std::make_unique<reckless::view::Button>(
        std::move(text), is_compact ? CompactChoicePainter() : theme::ChoicePainter(), std::move(clicked));
    AddChild(*plate);
    return plate;
}

void SidePanel::TellNames()
{
    if (on_names)
    {
        on_names(red_name_->text(), yellow_name_->text());
    }
}

void SidePanel::Show(const Controller &controller)
{
    const Settings &settings = controller.settings();
    const bool      is_two   = settings.is_two_player;
    if (is_two != is_two_player_)
    {
        is_two_player_ = is_two;
        Resized();
    }
    headline_->SetText(controller.StatusText());
    reckless::gfx::Color headline_colour = theme::kInk;
    if (controller.phase() == Phase::kWon)
    {
        headline_colour = is_two ? controller.ColoursOf(controller.winner()).light
                                 : (controller.winner() == kMin ? theme::kGood : theme::kBad);
    }
    headline_->SetColour(headline_colour);
    static constexpr std::array<const char *, 4> kLevelNames{"Easy", "Medium", "Hard", "Perfect"};
    const std::string board = std::to_string(settings.board.columns) + " × " + std::to_string(settings.board.rows);
    detail_->SetText(is_two ? "Two players  ·  " + board
                            : std::string(settings.is_person_red ? "You play red" : "You play yellow") + "  ·  " +
                                  kLevelNames[static_cast<std::size_t>(settings.level)] + "  ·  " + board);
    opponents_[0]->SetOn(!is_two);
    opponents_[1]->SetOn(is_two);
    for (std::size_t index = 0; index < levels_.size(); ++index)
    {
        levels_[index]->SetOn(static_cast<std::size_t>(settings.level) == index);
    }
    colours_[0]->SetOn(settings.is_person_red);
    colours_[1]->SetOn(!settings.is_person_red);
    // A name field being typed into is left to the person.
    if (!red_name_->has_focus() && red_name_->text() != settings.red_name)
    {
        red_name_->SetText(settings.red_name);
    }
    if (!yellow_name_->has_focus() && yellow_name_->text() != settings.yellow_name)
    {
        yellow_name_->SetText(settings.yellow_name);
    }
    starters_[0]->SetText(is_two ? "Red" : "You");
    starters_[1]->SetText(is_two ? "Yellow" : "Computer");
    starters_[0]->SetOn(settings.does_person_start);
    starters_[1]->SetOn(!settings.does_person_start);
    starters_[2]->SetOn(settings.does_alternate);
    for (std::size_t index = 0; index < boards_.size(); ++index)
    {
        boards_[index]->SetOn(settings.board == kBoardSizes[index]);
    }
    undo_->SetDimmed(!controller.CanUndo());
    hint_->SetDimmed(!controller.CanHint());
    hint_->SetText(controller.IsHintPending() ? "Hint…" : "Hint");
    sound_->SetOn(settings.has_sound);
    marks_->SetOn(settings.has_marks);
    score_->SetScore(controller.score(), is_two ? (settings.red_name.empty() ? "Red" : settings.red_name) : "You",
                     is_two ? (settings.yellow_name.empty() ? "Yellow" : settings.yellow_name) : "Computer", is_two);
}

void SidePanel::Paint(reckless::gfx::Canvas &c)
{
    c.FillAll(theme::kPanel);
    c.SetColor(theme::kRule);
    c.DrawVerticalLine(0, 0.0f, static_cast<float>(height()));
}

void SidePanel::PaintOverChildren(reckless::gfx::Canvas &c)
{
    // Each name field's puck beside it, in its player's colour and with its mark, so the names read as the colours.
    if (!is_two_player_)
    {
        return;
    }
    for (const auto &[field, colours] :
         {std::pair{red_name_.get(), theme::kRed}, std::pair{yellow_name_.get(), theme::kYellow}})
    {
        const RectI bounds = field->bounds();
        art::DrawPuck(c, {static_cast<float>(bounds.x()) - 16.0f, static_cast<float>(bounds.CentreY())}, 11.0f, colours,
                      true);
    }
}

void SidePanel::Resized()
{
    const int inner = width() - 2 * kMargin;
    int       y     = 18;
    title_->SetBounds({kMargin, y, inner, 20});
    y += 22;
    headline_->SetBounds({kMargin, y, inner, 40});
    y += 40;
    detail_->SetBounds({kMargin, y, inner, 22});
    y += 32;

    const int section_height =
        reckless::app::Section::kTitleHeight + kPlateHeight + 2 * reckless::app::Section::kPadding;
    // A section holding one row of plates, the row split evenly.
    const auto lay_row = [&](reckless::app::Section &section, auto begin, auto end) {
        section.SetVisible(true);
        section.SetBounds({kMargin, y, inner, section_height});
        const RectI area  = section.ContentArea().Translated(section.bounds().x(), section.bounds().y());
        const int   count = static_cast<int>(end - begin);
        const int   each  = (area.width() - (count - 1) * kGap) / count;
        int         index = 0;
        for (auto plate = begin; plate != end; ++plate, ++index)
        {
            (*plate)->SetVisible(true);
            (*plate)->SetBounds(
                {area.x() + index * (each + kGap), area.y() + (area.height() - kPlateHeight) / 2, each, kPlateHeight});
        }
        y += section_height + 8;
    };
    const auto hide = [](auto begin, auto end) {
        for (auto view = begin; view != end; ++view)
        {
            (*view)->SetVisible(false);
        }
    };

    lay_row(*opponent_section_, opponents_.begin(), opponents_.end());
    if (is_two_player_)
    {
        level_section_->SetVisible(false);
        colour_section_->SetVisible(false);
        hide(levels_.begin(), levels_.end());
        hide(colours_.begin(), colours_.end());
        // The names, each field with its colour's puck at its left.
        const int names_height =
            reckless::app::Section::kTitleHeight + 2 * kFieldHeight + kGap + 2 * reckless::app::Section::kPadding;
        names_section_->SetVisible(true);
        names_section_->SetBounds({kMargin, y, inner, names_height});
        const RectI area = names_section_->ContentArea().Translated(kMargin, y);
        red_name_->SetVisible(true);
        yellow_name_->SetVisible(true);
        red_name_->SetBounds({area.x() + 32, area.y(), area.width() - 32, kFieldHeight});
        yellow_name_->SetBounds({area.x() + 32, area.y() + kFieldHeight + kGap, area.width() - 32, kFieldHeight});
        y += names_height + 8;
        lay_row(*starter_section_, starters_.begin(), starters_.end());
    }
    else
    {
        names_section_->SetVisible(false);
        red_name_->SetVisible(false);
        yellow_name_->SetVisible(false);
        lay_row(*level_section_, levels_.begin(), levels_.end());
        lay_row(*colour_section_, colours_.begin(), colours_.end());
        lay_row(*starter_section_, starters_.begin(), starters_.begin() + 2);
        starters_[2]->SetVisible(false);
    }
    lay_row(*board_section_, boards_.begin(), boards_.end());

    // New game, wide, and Undo and Hint beside it.
    const int small = 72;
    new_game_->SetBounds({kMargin, y, inner - 2 * (small + kGap), 40});
    undo_->SetBounds({kMargin + inner - 2 * small - kGap, y, small, 40});
    hint_->SetBounds({kMargin + inner - small, y, small, 40});
    y += 40 + 12;

    const int score_height = reckless::app::Section::kTitleHeight + 58 + 2 * reckless::app::Section::kPadding;
    score_section_->SetBounds({kMargin, y, inner, score_height});
    const RectI score_area = score_section_->ContentArea().Translated(kMargin, y);
    score_->SetBounds({score_area.x(), score_area.y(), score_area.width(), 58});
    y += score_height + 10;

    const int toggles_y = std::max(y, height() - kMargin - kPlateHeight);
    sound_->SetBounds({kMargin, toggles_y, 110, kPlateHeight});
    marks_->SetBounds({kMargin + 110 + kGap, toggles_y, 110, kPlateHeight});
}

} // namespace knect4::app
