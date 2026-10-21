#pragma once
/// @file search.h The computer's play on one board size: an exact solver and depth-limited searches, every win scored
/// by how soon it comes, equal moves chosen between at random. engine.h puts the board sizes behind one interface.

#include "game/position.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <limits>
#include <optional>
#include <random>
#include <vector>

namespace knect4
{

/** How well the computer plays. */
enum class Strength
{
    kEasy,    ///< Two plies: takes a four it is offered, usually stops one, and otherwise plays loosely.
    kMedium,  ///< Up to six plies on a judgement of threats, now and then content with a lesser move.
    kHard,    ///< Up to twelve plies in the time allowed: sees most endgames through to their end.
    kPerfect, ///< The exact solution wherever it can be found in time; else the deepest search the time allows.
};

/** A move's value from the side to move's view. */
struct MoveValue
{
    int  column{0};       ///< The column.
    int  score{0};        ///< Exact: the game's score in kWinScale units; else a judgement.
    bool is_exact{false}; ///< Whether the score is the game's outcome rather than a judgement.
};

inline constexpr int kWinScale = 1000; ///< An exact score's unit, against a judgement's.

/**
 * The computer's play on positions of type `P` (a BasicPosition). Scores are from the side to move's view. An exact
 * score s > 0 is a win in which the winner has s - 1 stones still in hand when it makes four, so a sooner win scores
 * higher; s < 0 a loss, the latest scoring nearest 0; 0 a draw. A judged score lies strictly between -kWinScale and
 * kWinScale, so any exact win, in kWinScale units, outranks any judgement.
 *
 * One search runs at a time. The solver's table (about 40 MB) is kept between searches. Each search reads `stop` and
 * its own deadline as it goes and, stopped, answers with the best it has found.
 * @tparam P The position type.
 */
template <typename P> class Search
{
  public:
    using Bits                     = typename P::BitsType; ///< The position's bit set.
    static constexpr int kMaxScore = (P::kSlots + 1) / 2;  ///< The quickest win's exact score, in moves.

    /** @param seed The choice between equal moves; the same seed chooses the same way. */
    explicit Search(std::uint64_t seed) : random_(seed), keys_(kTableSize, 0), bounds_(kTableSize, 0)
    {
    }

    /**
     * The column the computer plays at `strength`.
     * @param position A position whose game is not over: the side to move has a move.
     * @param strength How well it plays.
     * @param stop Read as it searches: once true, the best found so far; may be null.
     * @param seconds The time it may take; 0 for the level's own (Hard 2 s, Perfect 4 s, the others not timed).
     * @return The column.
     */
    int ChooseMove(const P &position, Strength strength, const std::atomic<bool> *stop = nullptr, double seconds = 0.0)
    {
        const auto allowed = [seconds](double usual) { return seconds > 0.0 ? seconds : usual; };
        switch (strength)
        {
        case Strength::kEasy: {
            // Two plies with no judgement - it takes a four and stops one, and is otherwise aimless - and one move in
            // three it does not look past its own: it misses a threat it should have stopped.
            std::vector<MoveValue> values = Values(position, Strength::kEasy, allowed(1.0), stop);
            if (std::uniform_int_distribution<int>(0, 2)(random_) == 0)
            {
                for (MoveValue &value : values)
                {
                    value.score = position.IsWinningMove(value.column) ? value.score : 0;
                }
            }
            return ChooseAmong(values, 0);
        }
        case Strength::kMedium:
            // Six plies, and any move within a threat's worth of the best: it varies, and slips now and then.
            return ChooseAmong(Values(position, Strength::kMedium, allowed(2.0), stop), 40);
        case Strength::kHard:
            return ChooseAmong(Values(position, Strength::kHard, allowed(2.0), stop), 0);
        case Strength::kPerfect:
        default:
            return ChooseAmong(Values(position, Strength::kPerfect, allowed(4.0), stop), 0);
        }
    }

    /**
     * Every possible move's value, searched at `strength`'s depth or exactly, within `seconds`.
     * @param position A position whose game is not over.
     * @param strength How deep to look: kPerfect solves.
     * @param seconds The time allowed; a search not done by then gives its deepest finished judgement.
     * @param stop Read as it searches; may be null.
     * @return Each column with room and its value, in column order.
     */
    std::vector<MoveValue> Values(const P &position, Strength strength, double seconds,
                                  const std::atomic<bool> *stop = nullptr)
    {
        Begin(seconds, stop);
        std::vector<MoveValue> values;
        for (int column = 0; column != P::kWidth; ++column)
        {
            if (position.CanPlay(column))
            {
                values.push_back({column, 0, false});
            }
        }
        if (strength == Strength::kPerfect)
        {
            try
            {
                for (MoveValue &value : values)
                {
                    if (position.IsWinningMove(value.column))
                    {
                        value.score = WinNow(position) * kWinScale;
                    }
                    else
                    {
                        P child = position;
                        child.Play(value.column);
                        value.score = -SolveExactly(child) * kWinScale;
                    }
                    value.is_exact = true;
                }
                return values;
            }
            catch (const Abandoned &)
            {
                // Not solved in time: the deepest judgement that finishes in half as long again.
                deadline_ = After(seconds / 2.0);
                for (MoveValue &value : values)
                {
                    value = {value.column, 0, false};
                }
            }
        }
        const bool is_judged = strength != Strength::kEasy;
        const int  last_depth =
            strength == Strength::kEasy
                ? 2
                : (strength == Strength::kMedium ? 6 : (strength == Strength::kHard ? 12 : P::kSlots));
        // Every level deepens two plies at a time to its depth - until the time is gone or the game is seen to its end
        // - keeping the deepest search that finished: on the standard board Easy and Medium reach theirs at once, and
        // on the widest board the time decides.
        std::vector<MoveValue> finished = values;
        for (int depth = 2; depth <= last_depth; depth += 2)
        {
            try
            {
                std::vector<MoveValue> searched   = values;
                bool                   is_settled = true;
                for (MoveValue &value : searched)
                {
                    if (position.IsWinningMove(value.column))
                    {
                        value.score    = WinNow(position) * kWinScale;
                        value.is_exact = true;
                        continue;
                    }
                    P child = position;
                    child.Play(value.column);
                    value.score    = -SearchTo(child, depth - 1, -kInfinity, kInfinity, is_judged);
                    value.is_exact = value.score <= -kWinScale || value.score >= kWinScale;
                    is_settled     = is_settled && value.is_exact;
                }
                finished = searched;
                if (is_settled)
                {
                    break; // every move's outcome is known: deeper changes nothing
                }
            }
            catch (const Abandoned &)
            {
                break;
            }
        }
        return finished;
    }

    /**
     * The game's exact score with best play from both sides, if found within `seconds`.
     * @param position A position whose game is not over.
     * @param seconds The time allowed.
     * @param stop Read as it searches; may be null.
     * @return The score in moves (not kWinScale units): positive a win for the side to move, 0 a draw; none when not
     *         found in time.
     */
    std::optional<int> Solve(const P &position, double seconds, const std::atomic<bool> *stop = nullptr)
    {
        Begin(seconds, stop);
        try
        {
            return SolveExactly(position);
        }
        catch (const Abandoned &)
        {
            return std::nullopt;
        }
    }

    /**
     * The judged score of `position` from the side to move's view: its threats against the other side's, the threats
     * on the rows that tend to win for each side, and the centre.
     * @param position A position.
     * @return The score, strictly between -kWinScale and kWinScale.
     */
    static int Judge(const P &position) noexcept
    {
        const Bits mine   = position.WinningPositions();
        const Bits theirs = position.OpponentWinningPositions();
        // The first player tends to win on threats in odd rows (the 1st, 3rd... from the bottom), the second in even.
        const bool is_first   = position.moves() % 2 == 0;
        const Bits my_rows    = P::RowsOfParity(is_first ? 0 : 1);
        const Bits their_rows = P::RowsOfParity(is_first ? 1 : 0);
        int        score      = 40 * (CountBits(mine) - CountBits(theirs));
        score += 25 * (CountBits(mine & my_rows) - CountBits(theirs & their_rows));
        // The middle column, or the two of an even board, takes part in the most lines.
        const Bits centre = P::ColumnMask(P::kWidth / 2) | P::ColumnMask((P::kWidth - 1) / 2);
        score +=
            6 * (CountBits(position.current() & centre) - CountBits((position.current() ^ position.mask()) & centre));
        return std::clamp(score, -(kWinScale - 1), kWinScale - 1);
    }

    /** @return Positions the last search looked at. */
    std::uint64_t nodes() const noexcept
    {
        return nodes_;
    }

  private:
    static constexpr std::size_t kTableSize = 4194301; ///< The table's slots: a prime, so keys spread over them.
    static constexpr int         kInfinity  = std::numeric_limits<int>::max() / 2; ///< Beyond any score.

    /** Why a search gave up: stop was set, or its time ran out. */
    struct Abandoned
    {
    };

    /** Up to the board's width of moves kept sorted by score, ties in the order they came, handed back best first. */
    class MoveSorter
    {
      public:
        /** @param move A slot's bit. @param score How good it looks. */
        void Add(Bits move, int score) noexcept
        {
            int place = size_++;
            for (; place != 0 && entries_[static_cast<std::size_t>(place - 1)].score > score; --place)
            {
                entries_[static_cast<std::size_t>(place)] = entries_[static_cast<std::size_t>(place - 1)];
            }
            entries_[static_cast<std::size_t>(place)] = {move, score};
        }

        /** @param move Set to the best move left. @return Whether there was one. */
        bool Next(Bits &move) noexcept
        {
            if (size_ == 0)
            {
                return false;
            }
            move = entries_[static_cast<std::size_t>(--size_)].move;
            return true;
        }

      private:
        /** A move and its score. */
        struct Entry
        {
            Bits move{};   ///< The slot's bit.
            int  score{0}; ///< How good it looks.
        };

        std::array<Entry, P::kWidth> entries_{}; ///< The moves, worst first.
        int                          size_{0};   ///< How many.
    };

    /** The columns from the centre out: the order moves are tried in before their threats sort them. */
    static constexpr std::array<int, P::kWidth> kCentreFirst = [] {
        std::array<int, P::kWidth> order{};
        for (int index = 0; index != P::kWidth; ++index)
        {
            order[static_cast<std::size_t>(index)] = P::kWidth / 2 + (1 - 2 * (index % 2)) * (index + 1) / 2;
        }
        return order;
    }();

    /** @param seconds A span. @return The time it ends, from now. */
    static std::chrono::steady_clock::time_point After(double seconds)
    {
        const auto span = std::chrono::duration_cast<std::chrono::steady_clock::duration>(
            std::chrono::duration<double>(std::max(seconds, 0.0)));
        return std::chrono::steady_clock::now() + span;
    }

    /** @param position A position. @return The exact score, in moves, of the side to move winning with its next stone.
     */
    static int WinNow(const P &position) noexcept
    {
        return (P::kSlots + 1 - position.moves()) / 2;
    }

    /** A search begun: its limits set, its count cleared. @param seconds Its time. @param stop Its stop. */
    void Begin(double seconds, const std::atomic<bool> *stop)
    {
        stop_     = stop;
        deadline_ = After(seconds);
        nodes_    = 0;
    }

    /** @throw Abandoned when the search must stop. */
    void CheckLimits()
    {
        if ((nodes_ & 0xfff) == 0 && ((stop_ != nullptr && stop_->load(std::memory_order_relaxed)) ||
                                      std::chrono::steady_clock::now() > deadline_))
        {
            throw Abandoned{};
        }
    }

    /**
     * The moves of `next` in the order to try them: by the threats each makes, ties from the centre out.
     * @param position The position.
     * @param next The moves.
     * @return The sorter, best last.
     */
    static MoveSorter Ordered(const P &position, Bits next) noexcept
    {
        MoveSorter moves;
        for (int index = P::kWidth; index-- != 0;)
        {
            const Bits move = next & P::ColumnMask(kCentreFirst[static_cast<std::size_t>(index)]);
            if (Any(move))
            {
                moves.Add(move, position.MoveScore(move));
            }
        }
        return moves;
    }

    /**
     * The depth-limited search: exact at a four or a full board, judged (or 0, for Easy) at depth 0.
     * @param position The position.
     * @param depth Plies left.
     * @param alpha The window's lower edge.
     * @param beta Its upper edge.
     * @param is_judged Whether depth 0 judges (Judge) or scores every open position 0.
     * @return The score, in kWinScale units for an exact outcome.
     */
    int SearchTo(const P &position, int depth, int alpha, int beta, bool is_judged)
    {
        ++nodes_;
        CheckLimits();
        if (position.CanWinNext())
        {
            return WinNow(position) * kWinScale;
        }
        const Bits next = position.PossibleNonLosingMoves();
        if (!Any(next))
        {
            return -((P::kSlots - position.moves()) / 2) * kWinScale;
        }
        if (position.moves() >= P::kSlots - 2)
        {
            return 0; // the last two stones make no four: a draw
        }
        if (depth <= 0)
        {
            return is_judged ? Judge(position) : 0;
        }
        MoveSorter moves = Ordered(position, next);
        for (Bits move{}; moves.Next(move);)
        {
            P child = position;
            child.PlayMove(move);
            const int score = -SearchTo(child, depth - 1, -beta, -alpha, is_judged);
            if (score >= beta)
            {
                return score;
            }
            alpha = std::max(alpha, score);
        }
        return alpha;
    }

    /**
     * The exact solver's search with a window, Pascal Pons's design: only moves that do not lose at once, the table's
     * upper bounds, moves tried by the threats they make, the centre first.
     * @param position The position; the side to move cannot win at once.
     * @param alpha The window's lower edge, in moves.
     * @param beta Its upper edge.
     * @return The score in moves, within the window's meaning.
     */
    int Negamax(const P &position, int alpha, int beta)
    {
        ++nodes_;
        CheckLimits();
        const Bits next = position.PossibleNonLosingMoves();
        if (!Any(next))
        {
            return -(P::kSlots - position.moves()) / 2;
        }
        if (position.moves() >= P::kSlots - 2)
        {
            return 0;
        }
        // The other side cannot win at once, so the side to move loses no sooner than after its next two stones.
        const int least = -(P::kSlots - 2 - position.moves()) / 2;
        if (alpha < least)
        {
            alpha = least;
            if (alpha >= beta)
            {
                return alpha;
            }
        }
        // Nor can it win at once (the caller checked), so no sooner than after the other side's next stone.
        int                 most  = (P::kSlots - 1 - position.moves()) / 2;
        const std::uint64_t hash  = HashBits(position.Key());
        const std::size_t   index = static_cast<std::size_t>(hash % kTableSize);
        if (keys_[index] == hash && bounds_[index] != 0)
        {
            most = bounds_[index] - kMaxScore - 1;
        }
        if (beta > most)
        {
            beta = most;
            if (alpha >= beta)
            {
                return beta;
            }
        }
        MoveSorter moves = Ordered(position, next);
        for (Bits move{}; moves.Next(move);)
        {
            P child = position;
            child.PlayMove(move);
            const int score = -Negamax(child, -beta, -alpha);
            if (score >= beta)
            {
                return score;
            }
            alpha = std::max(alpha, score);
        }
        keys_[index]   = hash;
        bounds_[index] = static_cast<std::uint8_t>(alpha + kMaxScore + 1);
        return alpha;
    }

    /** @param position A position whose game is not over. @return Its exact score in moves. */
    int SolveExactly(const P &position)
    {
        if (position.CanWinNext())
        {
            return WinNow(position);
        }
        // Narrowing null windows, nearer 0 first: a draw or a near result is cheapest to prove.
        int least = -(P::kSlots - position.moves()) / 2;
        int most  = (P::kSlots + 1 - position.moves()) / 2;
        while (least < most)
        {
            int middle = least + (most - least) / 2;
            if (middle <= 0 && least / 2 < middle)
            {
                middle = least / 2;
            }
            else if (middle >= 0 && most / 2 > middle)
            {
                middle = most / 2;
            }
            const int result = Negamax(position, middle, middle + 1);
            if (result <= middle)
            {
                most = result;
            }
            else
            {
                least = result;
            }
        }
        return least;
    }

    /**
     * A move chosen among `values`: of those within `margin` of the best, one at random.
     * @param values The moves' values; not empty.
     * @param margin How far below the best a move may be.
     * @return The column.
     */
    int ChooseAmong(const std::vector<MoveValue> &values, int margin)
    {
        int best = -kInfinity;
        for (const MoveValue &value : values)
        {
            best = std::max(best, value.score);
        }
        std::vector<int> choices;
        for (const MoveValue &value : values)
        {
            if (value.score >= best - margin)
            {
                choices.push_back(value.column);
            }
        }
        return choices[std::uniform_int_distribution<std::size_t>(0, choices.size() - 1)(random_)];
    }

    std::mt19937_64                       random_;        ///< The choice between equal moves.
    std::vector<std::uint64_t>            keys_;          ///< The table: each slot's key's hash.
    std::vector<std::uint8_t>             bounds_;        ///< Each slot's upper bound, kMaxScore + 1 above it; 0 none.
    const std::atomic<bool>              *stop_{nullptr}; ///< The search's stop.
    std::chrono::steady_clock::time_point deadline_{};    ///< When the search gives up.
    std::uint64_t                         nodes_{0};      ///< Positions looked at.
};

} // namespace knect4
