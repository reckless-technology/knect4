#include "app/controller.h"

#include "app/board_art.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <numbers>

namespace connect4::app
{

namespace
{

/**
 * How far the held puck glides toward its target in `seconds`: most of the way in a tenth of a second.
 * @param seconds The time since the last frame.
 * @param rate How quickly, per second.
 * @return The share of the way to go, 0 to 1.
 */
float Glide(double seconds, double rate)
{
    return static_cast<float>(1.0 - std::exp(-std::max(seconds, 0.0) * rate));
}

/**
 * How far a puck falls from where it is held to `row`, in rows.
 * @param row The row it lands in.
 * @return The distance.
 */
double FallTo(int row)
{
    return static_cast<double>((art::RowY(row) - art::kHoverY) / art::kPitch);
}

} // namespace

int DepthOf(Level level) noexcept
{
    switch (level)
    {
    case Level::kEasy:
        return 3;
    case Level::kHard:
        return 11;
    case Level::kMedium:
    default:
        return 7;
    }
}

Controller::Controller(ClickSink *clicks, Settings settings) : clicks_(clicks), settings_(settings)
{
    NewGame();
}

Controller::~Controller()
{
    StopSearch();
}

void Controller::NewGame()
{
    StopSearch();
    game_.Reset();
    motion_.reset();
    winning_.clear();
    glow_time_       = 0.0;
    moves_made_      = 0;
    chosen_          = -1;
    computer_column_ = 3;
    if (settings_.does_person_start)
    {
        BeginPersonTurn();
    }
    else
    {
        BeginComputerTurn();
    }
}

void Controller::BeginPersonTurn()
{
    phase_ = Phase::kPersonToMove;
    // The person's puck appears where the pointer is, fading in, rather than gliding over from the computer's.
    held_column_  = static_cast<float>(held_target_);
    held_opacity_ = 0.0f;
}

void Controller::BeginComputerTurn()
{
    phase_        = Phase::kComputerThinking;
    turn_time_    = 0.0;
    chosen_       = -1;
    held_column_  = static_cast<float>(computer_column_);
    held_opacity_ = 0.0f;
    stop_         = std::make_shared<std::atomic<bool>>(false);
    // The search on a copy of the board, so the window's thread never waits on it.
    search_ = std::async(std::launch::async, [board = game_, depth = DepthOf(settings_.level), stop = stop_]() mutable {
        int move = -1;
        board.NextMove(depth, kMax, move, stop.get());
        return move;
    });
}

void Controller::StopSearch()
{
    if (search_.valid())
    {
        stop_->store(true);
        search_.wait();
        search_ = {};
    }
}

int Controller::NearestOpenColumn(int from, int delta) const
{
    for (int step = 0; step != game_.width(); ++step)
    {
        const int column = ((from + delta * step) % game_.width() + game_.width()) % game_.width();
        if (game_.IsMovePossible(column))
        {
            return column;
        }
    }
    return from;
}

void Controller::Hover(int column)
{
    if (column < 0)
    {
        is_held_shown_ = false;
        return;
    }
    is_held_shown_ = true;
    held_target_   = column;
}

bool Controller::Drop(int column)
{
    if (phase_ != Phase::kPersonToMove || !game_.IsMovePossible(column))
    {
        return false;
    }
    held_target_ = column;
    StartDrop(column, kMin);
    return true;
}

void Controller::Step(int delta)
{
    if (phase_ != Phase::kPersonToMove)
    {
        return;
    }
    const int from = is_held_shown_ ? held_target_ + delta : held_target_;
    held_target_   = NearestOpenColumn(std::clamp(from, 0, game_.width() - 1), delta < 0 ? -1 : 1);
    is_held_shown_ = true;
}

bool Controller::DropHeld()
{
    is_held_shown_ = true;
    return Drop(held_target_);
}

void Controller::PlaceMoves(const std::vector<int> &columns)
{
    NewGame();
    StopSearch();
    int player = settings_.does_person_start ? kMin : kMax;
    for (const int column : columns)
    {
        if (!game_.IsMovePossible(column))
        {
            continue;
        }
        game_.DoMove(column, player);
        ++moves_made_;
        falling_column_ = column;
        falling_player_ = player;
        if (player == kMax)
        {
            computer_column_ = column;
        }
        player = -player;
    }
    if (moves_made_ > 0)
    {
        Landed();
    }
    else if (!settings_.does_person_start)
    {
        BeginComputerTurn(); // the search NewGame started, started again
    }
}

void Controller::StartDrop(int column, int player)
{
    const int row = game_.ColumnHeight(column);
    game_.DoMove(column, player);
    ++moves_made_;
    falling_column_ = column;
    falling_player_ = player;
    motion_.emplace(FallTo(row));
    phase_ = Phase::kDropping;
    if (player == kMax)
    {
        computer_column_ = column;
    }
}

void Controller::Landed()
{
    motion_.reset();
    const int column = falling_column_;
    const int player = falling_player_;
    if (game_.IsWinningMove(column, player))
    {
        winning_ = game_.WinningLine(column);
        if (player == kMin)
        {
            phase_ = Phase::kPersonWon;
            ++score_.person;
        }
        else
        {
            phase_ = Phase::kComputerWon;
            ++score_.computer;
        }
        glow_time_ = 0.0;
        return;
    }
    if (game_.IsDraw(column, player))
    {
        phase_ = Phase::kDrawn;
        ++score_.drawn;
        return;
    }
    if (player == kMin)
    {
        BeginComputerTurn();
    }
    else
    {
        BeginPersonTurn();
    }
}

void Controller::Frame(double seconds)
{
    seconds = std::clamp(seconds, 0.0, 0.25); // a stall, a breakpoint: the game takes it as a quarter of a second
    switch (phase_)
    {
    case Phase::kPersonToMove:
        held_column_ += (static_cast<float>(held_target_) - held_column_) * Glide(seconds, 22.0);
        held_opacity_ = is_held_shown_ ? std::min(held_opacity_ + static_cast<float>(seconds) * 6.0f, 1.0f)
                                       : std::max(held_opacity_ - static_cast<float>(seconds) * 6.0f, 0.0f);
        break;
    case Phase::kComputerThinking:
        turn_time_ += seconds;
        held_opacity_ = std::min(held_opacity_ + static_cast<float>(seconds) * 5.0f, 1.0f);
        if (search_.valid() && search_.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
        {
            chosen_ = search_.get();
            search_ = {};
            if (!game_.IsMovePossible(chosen_))
            {
                chosen_ = NearestOpenColumn(0, 1); // never: the search always finds a move on a board with room
            }
        }
        if (chosen_ >= 0 && turn_time_ >= kLeastThinking)
        {
            phase_ = Phase::kComputerAiming;
        }
        break;
    case Phase::kComputerAiming:
        turn_time_ += seconds;
        held_opacity_ = std::min(held_opacity_ + static_cast<float>(seconds) * 5.0f, 1.0f);
        held_column_ += (static_cast<float>(chosen_) - held_column_) * Glide(seconds, 12.0);
        if (std::abs(held_column_ - static_cast<float>(chosen_)) < 0.03f)
        {
            held_column_ = static_cast<float>(chosen_);
            StartDrop(chosen_, kMax);
        }
        break;
    case Phase::kDropping: {
        motion_->Advance(seconds);
        const double full_speed = DropMotion::HitSpeed(FallTo(0), {});
        for (const DropMotion::Impact &impact : motion_->TakeImpacts())
        {
            if (clicks_ != nullptr && settings_.has_sound)
            {
                // The first hit louder the further it fell, each bounce's softer.
                const float share = static_cast<float>(std::clamp(impact.speed / full_speed, 0.0, 1.0));
                const float gain  = impact.number == 0 ? 0.45f + 0.55f * share : 0.9f * share;
                const int   row   = game_.ColumnHeight(falling_column_) - 1;
                clicks_->Click(row == 0 ? LandingOn::kBase : LandingOn::kPuck, gain);
            }
        }
        if (motion_->has_settled())
        {
            Landed();
        }
        break;
    }
    case Phase::kPersonWon:
    case Phase::kComputerWon:
    case Phase::kDrawn:
        glow_time_ += seconds;
        break;
    }
}

void Controller::SetLevel(Level level)
{
    settings_.level = level;
}

void Controller::SetPersonRed(bool is_person_red)
{
    settings_.is_person_red = is_person_red;
}

void Controller::SetPersonStarts(bool does_person_start)
{
    settings_.does_person_start = does_person_start;
}

void Controller::SetSound(bool has_sound)
{
    settings_.has_sound = has_sound;
}

BoardPicture Controller::Picture() const
{
    BoardPicture picture;
    for (int column = 0; column != game_.width(); ++column)
    {
        for (int row = 0; row != game_.height(); ++row)
        {
            picture.pieces[static_cast<std::size_t>(column)][static_cast<std::size_t>(row)] = game_.At(column, row);
        }
    }
    picture.person   = settings_.is_person_red ? theme::kRed : theme::kYellow;
    picture.computer = settings_.is_person_red ? theme::kYellow : theme::kRed;
    if (motion_)
    {
        // The falling puck is in the game already; it shows falling, not at rest.
        const int row = game_.ColumnHeight(falling_column_) - 1;
        picture.pieces[static_cast<std::size_t>(falling_column_)][static_cast<std::size_t>(row)] = kEmpty;
        picture.falling = FallingPuck{falling_column_, motion_->position(), falling_player_};
    }
    switch (phase_)
    {
    case Phase::kPersonToMove:
        picture.may_choose = true;
        if (held_opacity_ > 0.0f)
        {
            picture.held = HeldPuck{held_column_, kMin, is_held_shown_ ? held_target_ : -1, held_opacity_};
        }
        break;
    case Phase::kComputerThinking: {
        // A gentle sway while it thinks.
        const float sway = 0.12f * static_cast<float>(std::sin(turn_time_ * 2.0 * std::numbers::pi * 0.9));
        picture.held = HeldPuck{held_column_ + sway * std::min(static_cast<float>(turn_time_) * 3.0f, 1.0f), kMax, -1,
                                held_opacity_};
        break;
    }
    case Phase::kComputerAiming:
        picture.held = HeldPuck{held_column_, kMax, -1, held_opacity_};
        break;
    case Phase::kDropping:
        break;
    case Phase::kPersonWon:
    case Phase::kComputerWon:
    case Phase::kDrawn:
        picture.winning = winning_;
        picture.glow    = static_cast<float>(0.5 + 0.5 * std::cos(glow_time_ * 2.0 * std::numbers::pi * 0.8));
        picture.banner  = StatusText();
        picture.banner_colour =
            phase_ == Phase::kPersonWon ? theme::kGood : (phase_ == Phase::kComputerWon ? theme::kBad : theme::kInk);
        break;
    }
    return picture;
}

std::string Controller::StatusText() const
{
    switch (phase_)
    {
    case Phase::kPersonToMove:
        return "Your move";
    case Phase::kComputerThinking:
    case Phase::kComputerAiming:
        return "Thinking…";
    case Phase::kDropping:
        return falling_player_ == kMin ? "Your move" : "Thinking…";
    case Phase::kPersonWon:
        return "You win!";
    case Phase::kComputerWon:
        return "I win";
    case Phase::kDrawn:
    default:
        return "A draw";
    }
}

} // namespace connect4::app
