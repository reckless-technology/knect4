#include "app/controller.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <numbers>
#include <utility>

namespace knect4::app
{

namespace
{

constexpr double kEmptyingGravity = 90.0;  ///< The let-out pucks' fall, in rows a second a second.
constexpr double kSlideSeconds    = 0.15;  ///< The slider's travel each way.
constexpr float  kGoneBelow       = -1.6f; ///< The row past which a let-out puck has left the board.

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
 * When a column's let-out pucks start to fall, after the slider opens: left to right, a sweep across the board.
 * @param column The column.
 * @param columns The board's columns.
 * @return Seconds after the slider began to open.
 */
double ReleaseDelay(int column, int columns)
{
    return kSlideSeconds + 0.35 * static_cast<double>(column) / static_cast<double>(std::max(columns, 1));
}

/**
 * How far a let-out puck has fallen at `time`.
 * @param time Seconds since the slider began to open.
 * @param delay When its column began to fall.
 * @return Rows fallen.
 */
float Fallen(double time, double delay)
{
    const double falling = std::max(time - delay, 0.0);
    return static_cast<float>(0.5 * kEmptyingGravity * falling * falling);
}

} // namespace

Strength StrengthOf(Level level) noexcept
{
    switch (level)
    {
    case Level::kEasy:
        return Strength::kEasy;
    case Level::kHard:
        return Strength::kHard;
    case Level::kPerfect:
        return Strength::kPerfect;
    case Level::kMedium:
    default:
        return Strength::kMedium;
    }
}

Controller::Controller(ClickSink *clicks, Settings settings, Scores scores)
    : clicks_(clicks), settings_(std::move(settings)), scores_(scores)
{
    if (!IsOffered(settings_.board))
    {
        settings_.board = kStandardBoard;
    }
    MakeBoard();
    StartGame();
}

Controller::~Controller()
{
    StopSearch();
}

void Controller::MakeBoard()
{
    game_            = Game(settings_.board.columns, settings_.board.rows);
    layout_          = art::Layout(settings_.board.columns, settings_.board.rows);
    held_target_     = std::clamp(held_target_, 0, game_.width() - 1);
    held_column_     = static_cast<float>(held_target_);
    computer_column_ = game_.width() / 2;
}

Engine &Controller::EngineForBoard()
{
    if (!engine_ || !(engine_->size() == settings_.board))
    {
        engine_ = std::make_unique<Engine>(settings_.board);
    }
    return *engine_;
}

void Controller::NewGame()
{
    StopSearch();
    if (phase_ == Phase::kEmptying)
    {
        return; // already on its way to a new game
    }
    // The pucks on the board, as they stand (a falling puck where it will land), let out through the slider.
    outgoing_.clear();
    for (int column = 0; column != game_.width(); ++column)
    {
        for (int row = 0; row != game_.ColumnHeight(column); ++row)
        {
            outgoing_.push_back({column, row, game_.At(column, row)});
        }
    }
    game_.Reset();
    moves_.clear();
    motion_.reset();
    winning_.clear();
    hint_column_ = -1;
    if (outgoing_.empty())
    {
        StartGame();
        return;
    }
    phase_               = Phase::kEmptying;
    emptying_time_       = 0.0;
    does_next_alternate_ = true; // a game was played on this board
    has_clattered_.assign(static_cast<std::size_t>(game_.width()), false);
    if (clicks_ != nullptr && settings_.has_sound)
    {
        clicks_->Prepare();
    }
}

void Controller::StartGame()
{
    StopSearch();
    game_.Reset();
    moves_.clear();
    motion_.reset();
    winning_.clear();
    outgoing_.clear();
    glow_time_   = 0.0;
    chosen_      = -1;
    hint_column_ = -1;
    // Between two people the first move passes to the other each new game after one played, where they asked for it.
    if (settings_.is_two_player && settings_.does_alternate && does_next_alternate_)
    {
        settings_.does_person_start = !settings_.does_person_start;
    }
    does_next_alternate_ = false;
    computer_column_     = game_.width() / 2;
    first_player_        = settings_.does_person_start ? kMin : kMax;
    if (first_player_ == kMin || settings_.is_two_player)
    {
        BeginPersonTurn(first_player_);
    }
    else
    {
        BeginComputerTurn();
    }
}

void Controller::BeginPersonTurn(int player)
{
    phase_ = Phase::kPersonToMove;
    mover_ = player;
    // A puck held over a column filled since is moved to the nearest that has room, so the keys drop it at once.
    held_target_ = NearestOpenColumn(std::clamp(held_target_, 0, game_.width() - 1));
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
    search_kind_  = SearchKind::kMove;
    // The search on its own thread with a copy of the moves, so the window's thread never waits on it; one search runs
    // at a time, so the engine is the search's alone until it ends or is stopped.
    search_ = std::async(std::launch::async,
                         [engine = &EngineForBoard(), moves = moves_, strength = StrengthOf(settings_.level),
                          stop = stop_] { return engine->ChooseMove(moves, strength, stop.get()); });
}

void Controller::StopSearch()
{
    if (search_.valid())
    {
        stop_->store(true);
        search_.wait();
        search_ = {};
    }
    search_kind_ = SearchKind::kNone;
}

int Controller::OpenColumnFrom(int from, int delta) const
{
    for (int column = from; column >= 0 && column < game_.width(); column += delta)
    {
        if (game_.IsMovePossible(column))
        {
            return column;
        }
    }
    return -1;
}

int Controller::NearestOpenColumn(int column) const
{
    for (int distance = 0; distance != game_.width(); ++distance)
    {
        for (const int candidate : {column + distance, column - distance})
        {
            if (game_.IsMovePossible(candidate))
            {
                return candidate;
            }
        }
    }
    return column;
}

void Controller::Hover(int column)
{
    if (column < 0)
    {
        is_held_shown_ = false;
        return;
    }
    is_held_shown_ = true;
    is_keyboard_   = false;
    held_target_   = std::clamp(column, 0, game_.width() - 1);
}

bool Controller::Drop(int column)
{
    if (phase_ != Phase::kPersonToMove || !game_.IsMovePossible(column))
    {
        return false;
    }
    held_target_ = column;
    StartDrop(column, mover_);
    return true;
}

void Controller::Step(int delta)
{
    if (phase_ != Phase::kPersonToMove)
    {
        return;
    }
    const int step = delta < 0 ? -1 : 1;
    // A puck not yet shown appears where it is if it can drop there; one shown moves at least a column.
    const int from = is_held_shown_ ? held_target_ + step : held_target_;
    if (const int column = OpenColumnFrom(from, step); column >= 0)
    {
        held_target_ = column;
    }
    is_held_shown_ = true;
    is_keyboard_   = true;
}

bool Controller::DropHeld()
{
    is_held_shown_ = true;
    is_keyboard_   = true;
    return Drop(held_target_);
}

bool Controller::CanUndo() const noexcept
{
    if (moves_.empty() || phase_ == Phase::kDropping || phase_ == Phase::kEmptying)
    {
        return false;
    }
    if (settings_.is_two_player)
    {
        return true;
    }
    // Against the computer something of the person's must be there to take back.
    for (int index = 0; index != MovesMade(); ++index)
    {
        if (PlayerOfMove(index) == kMin)
        {
            return true;
        }
    }
    return false;
}

bool Controller::Undo()
{
    if (!CanUndo())
    {
        return false;
    }
    StopSearch();
    if (phase_ == Phase::kWon)
    {
        --(falling_player_ == kMin ? CurrentScore().min_wins : CurrentScore().max_wins);
    }
    else if (phase_ == Phase::kDrawn)
    {
        --CurrentScore().drawn;
    }
    const auto take_back = [this] {
        game_.UndoMove(moves_.back());
        moves_.pop_back();
    };
    if (settings_.is_two_player)
    {
        take_back();
    }
    else
    {
        // Back to the person's move before their last: the computer's answer first, where it made one.
        if (PlayerOfMove(MovesMade() - 1) == kMax)
        {
            take_back();
        }
        take_back();
    }
    winning_.clear();
    hint_column_ = -1;
    motion_.reset();
    const int to_move = PlayerOfMove(MovesMade());
    if (!moves_.empty())
    {
        falling_column_ = moves_.back();
        falling_player_ = -to_move;
    }
    BeginPersonTurn(to_move);
    held_opacity_ = 1.0f; // the hand is still there: no fade
    return true;
}

bool Controller::CanHint() const noexcept
{
    return phase_ == Phase::kPersonToMove && hint_column_ < 0 && search_kind_ == SearchKind::kNone;
}

bool Controller::IsHintPending() const noexcept
{
    return search_kind_ == SearchKind::kHint;
}

bool Controller::AskHint()
{
    if (!CanHint())
    {
        return false;
    }
    stop_        = std::make_shared<std::atomic<bool>>(false);
    search_kind_ = SearchKind::kHint;
    search_      = std::async(std::launch::async, [engine = &EngineForBoard(), moves = moves_, stop = stop_] {
        return engine->ChooseMove(moves, Strength::kPerfect, stop.get(), kHintSeconds);
    });
    return true;
}

void Controller::PlaceMoves(const std::vector<int> &columns)
{
    StopSearch();
    StartGame();
    StopSearch();
    for (const int column : columns)
    {
        if (!game_.IsMovePossible(column))
        {
            continue;
        }
        const int player = PlayerOfMove(MovesMade());
        game_.DoMove(column, player);
        moves_.push_back(column);
        falling_column_ = column;
        falling_player_ = player;
        if (player == kMax && !settings_.is_two_player)
        {
            computer_column_ = column;
        }
    }
    if (!moves_.empty())
    {
        Landed();
    }
    else if (first_player_ == kMax && !settings_.is_two_player)
    {
        BeginComputerTurn(); // the search StartGame began, begun again
    }
}

void Controller::StartDrop(int column, int player)
{
    StopSearch(); // a hint being searched for is no longer wanted
    hint_column_  = -1;
    const int row = game_.ColumnHeight(column);
    game_.DoMove(column, player);
    moves_.push_back(column);
    falling_column_ = column;
    falling_player_ = player;
    motion_.emplace(layout_.FallTo(row));
    phase_ = Phase::kDropping;
    if (clicks_ != nullptr && settings_.has_sound)
    {
        clicks_->Prepare(); // the fall takes long enough for the output to open
    }
    if (player == kMax && !settings_.is_two_player)
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
        phase_   = Phase::kWon;
        ++(player == kMin ? CurrentScore().min_wins : CurrentScore().max_wins);
        glow_time_ = 0.0;
        return;
    }
    if (game_.IsDraw(column, player))
    {
        phase_ = Phase::kDrawn;
        ++CurrentScore().drawn;
        return;
    }
    if (settings_.is_two_player || player == kMax)
    {
        BeginPersonTurn(-player);
    }
    else
    {
        BeginComputerTurn();
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
        if (search_kind_ == SearchKind::kHint && search_.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
        {
            const int column = search_.get();
            search_          = {};
            search_kind_     = SearchKind::kNone;
            hint_column_     = game_.IsMovePossible(column) ? column : -1;
        }
        break;
    case Phase::kComputerThinking:
        turn_time_ += seconds;
        held_opacity_ = std::min(held_opacity_ + static_cast<float>(seconds) * 5.0f, 1.0f);
        if (search_.valid() && search_.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
        {
            chosen_      = search_.get();
            search_      = {};
            search_kind_ = SearchKind::kNone;
            if (!game_.IsMovePossible(chosen_))
            {
                chosen_ = NearestOpenColumn(game_.width() / 2); // never: the search finds a move on a board with room
            }
        }
        if (chosen_ >= 0 && turn_time_ >= kLeastThinking)
        {
            // Aiming begins from where the swaying puck stands, so it glides on rather than jumping back first.
            held_column_ += Sway();
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
        const double full_speed = DropMotion::HitSpeed(layout_.FallTo(0), {});
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
    case Phase::kWon:
    case Phase::kDrawn:
        glow_time_ += seconds;
        break;
    case Phase::kEmptying: {
        emptying_time_ += seconds;
        // Each column's pucks clatter onto what is under the board as the lowest of them leaves the frame.
        bool is_any_left = false;
        for (const OutgoingPuck &puck : outgoing_)
        {
            const float row =
                static_cast<float>(puck.row) - Fallen(emptying_time_, ReleaseDelay(puck.column, game_.width()));
            is_any_left       = is_any_left || row > kGoneBelow;
            const auto column = static_cast<std::size_t>(puck.column);
            if (puck.row == 0 && row < -0.7f && !has_clattered_[column])
            {
                has_clattered_[column] = true;
                if (clicks_ != nullptr && settings_.has_sound)
                {
                    clicks_->Click(LandingOn::kBase, 0.35f);
                }
            }
        }
        // Once all are out, the slider shuts again and the new game begins.
        const double out_by = ReleaseDelay(game_.width() - 1, game_.width()) +
                              std::sqrt(2.0 * (static_cast<double>(game_.height()) - kGoneBelow) / kEmptyingGravity);
        if (!is_any_left && emptying_time_ >= out_by + kSlideSeconds)
        {
            StartGame();
        }
        break;
    }
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

void Controller::SetAlternate(bool does_alternate)
{
    settings_.does_alternate = does_alternate;
}

void Controller::SetSound(bool has_sound)
{
    settings_.has_sound = has_sound;
}

void Controller::SetMarks(bool has_marks)
{
    settings_.has_marks = has_marks;
}

void Controller::SetTwoPlayer(bool is_two_player)
{
    if (is_two_player != settings_.is_two_player)
    {
        settings_.is_two_player = is_two_player;
        NewGame();
        does_next_alternate_ = false; // the first game of the new kind starts as chosen
    }
}

void Controller::SetBoardSize(BoardSize size)
{
    if (!IsOffered(size) || size == settings_.board)
    {
        return;
    }
    StopSearch();
    settings_.board = size;
    phase_          = Phase::kPersonToMove; // the old board's pucks are not let out: the board itself changes
    MakeBoard();
    StartGame();
}

void Controller::SetNames(std::string red_name, std::string yellow_name)
{
    settings_.red_name    = std::move(red_name);
    settings_.yellow_name = std::move(yellow_name);
}

void Controller::SetWindowPercent(int percent)
{
    settings_.window_percent = percent;
}

float Controller::Sway() const noexcept
{
    // A gentle sway while the computer thinks, growing in over its first third of a second.
    const float sway = 0.12f * static_cast<float>(std::sin(turn_time_ * 2.0 * std::numbers::pi * 0.9));
    return sway * std::min(static_cast<float>(turn_time_) * 3.0f, 1.0f);
}

Score &Controller::CurrentScore() noexcept
{
    return settings_.is_two_player ? scores_.two_player : scores_.computer;
}

const theme::PuckColours &Controller::ColoursOf(int player) const noexcept
{
    // Between two people Red is always kMin; against the computer the person's colour is their choice.
    const bool is_red = settings_.is_two_player ? player == kMin : (player == kMin) == settings_.is_person_red;
    return is_red ? theme::kRed : theme::kYellow;
}

std::string Controller::NameOf(int player) const
{
    const std::string &name = player == kMin ? settings_.red_name : settings_.yellow_name;
    return name.empty() ? std::string(player == kMin ? "Red" : "Yellow") : name;
}

BoardPicture Controller::Picture() const
{
    BoardPicture picture;
    picture.layout = layout_;
    picture.pieces.assign(static_cast<std::size_t>(game_.width() * game_.height()), kEmpty);
    for (int column = 0; column != game_.width(); ++column)
    {
        for (int row = 0; row != game_.height(); ++row)
        {
            picture.pieces[static_cast<std::size_t>(column * game_.height() + row)] = game_.At(column, row);
        }
    }
    picture.person      = ColoursOf(kMin);
    picture.computer    = ColoursOf(kMax);
    picture.has_marks   = settings_.has_marks;
    picture.hint_column = hint_column_;
    if (motion_)
    {
        // The falling puck is in the game already; it shows falling, not at rest.
        const int row = game_.ColumnHeight(falling_column_) - 1;
        picture.pieces[static_cast<std::size_t>(falling_column_ * game_.height() + row)] = kEmpty;
        picture.falling = FallingPuck{falling_column_, motion_->position(), falling_player_};
    }
    switch (phase_)
    {
    case Phase::kPersonToMove:
        picture.may_choose = true;
        if (held_opacity_ > 0.0f)
        {
            picture.held = HeldPuck{held_column_, mover_, is_held_shown_ ? held_target_ : -1, held_opacity_,
                                    is_keyboard_ && is_held_shown_};
        }
        break;
    case Phase::kComputerThinking:
        picture.held = HeldPuck{held_column_ + Sway(), kMax, -1, held_opacity_, false};
        break;
    case Phase::kComputerAiming:
        picture.held = HeldPuck{held_column_, kMax, -1, held_opacity_, false};
        break;
    case Phase::kDropping:
        break;
    case Phase::kWon:
    case Phase::kDrawn:
        picture.winning = winning_;
        picture.glow    = static_cast<float>(0.5 + 0.5 * std::cos(glow_time_ * 2.0 * std::numbers::pi * 0.8));
        picture.banner  = StatusText();
        if (phase_ == Phase::kDrawn)
        {
            picture.banner_colour = theme::kInk;
        }
        else if (settings_.is_two_player)
        {
            picture.banner_colour = ColoursOf(winner()).light; // between two people, the winner's own colour
        }
        else
        {
            picture.banner_colour = winner() == kMin ? theme::kGood : theme::kBad;
        }
        break;
    case Phase::kEmptying: {
        // The slider opens, the pucks fall out of the frame's foot column by column, fading as they leave, and the
        // slider shuts behind them.
        const double out_by   = ReleaseDelay(game_.width() - 1, game_.width()) +
                                std::sqrt(2.0 * (static_cast<double>(game_.height()) - kGoneBelow) / kEmptyingGravity);
        const double shutting = emptying_time_ - out_by;
        picture.slide         = static_cast<float>(shutting > 0.0 ? std::max(1.0 - shutting / kSlideSeconds, 0.0)
                                                                  : std::min(emptying_time_ / kSlideSeconds, 1.0));
        for (const OutgoingPuck &puck : outgoing_)
        {
            const float row =
                static_cast<float>(puck.row) - Fallen(emptying_time_, ReleaseDelay(puck.column, game_.width()));
            if (row > kGoneBelow)
            {
                picture.emptying.push_back({puck.column, row, puck.player, std::clamp(row - kGoneBelow, 0.0f, 1.0f)});
            }
        }
        break;
    }
    }
    return picture;
}

std::string Controller::StatusText() const
{
    if (settings_.is_two_player)
    {
        switch (phase_)
        {
        case Phase::kPersonToMove:
            return NameOf(mover_) + " to move";
        case Phase::kDropping:
            return NameOf(falling_player_) + " to move";
        case Phase::kWon:
            return NameOf(winner()) + " wins!";
        case Phase::kEmptying:
            return "New game";
        case Phase::kDrawn:
        default:
            return "A draw";
        }
    }
    switch (phase_)
    {
    case Phase::kPersonToMove:
        return IsHintPending() ? "Looking for a hint…" : "Your move";
    case Phase::kComputerThinking:
    case Phase::kComputerAiming:
        return "Thinking…";
    case Phase::kDropping:
        return falling_player_ == kMin ? "Your move" : "Thinking…";
    case Phase::kWon:
        return winner() == kMin ? "You win!" : "I win";
    case Phase::kEmptying:
        return "New game";
    case Phase::kDrawn:
    default:
        return "A draw";
    }
}

} // namespace knect4::app
