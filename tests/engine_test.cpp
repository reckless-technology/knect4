#include "game/bits.h"
#include "game/engine.h"
#include "game/game.h"
#include "game/position.h"
#include "game/search.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <random>
#include <set>
#include <thread>
#include <vector>

namespace knect4
{
namespace
{

/**
 * @param columns Moves from the empty board.
 * @return The position after them.
 */
Position After(const std::vector<int> &columns)
{
    int        played   = 0;
    const auto position = Position::FromMoves(columns, &played);
    EXPECT_EQ(played, static_cast<int>(columns.size()));
    return position;
}

/**
 * The exact score by exhaustive search, with no pruning and no table: what the solver must agree with.
 * @tparam P The position type.
 * @param position A position whose game is not over.
 * @return The score in moves, as Search::Solve gives it.
 */
template <typename P> int Exhaustive(const P &position)
{
    if (position.moves() == P::kSlots)
    {
        return 0;
    }
    for (int column = 0; column != P::kWidth; ++column)
    {
        if (position.CanPlay(column) && position.IsWinningMove(column))
        {
            return (P::kSlots + 1 - position.moves()) / 2;
        }
    }
    int best = -P::kSlots;
    for (int column = 0; column != P::kWidth; ++column)
    {
        if (position.CanPlay(column))
        {
            P child = position;
            child.Play(column);
            best = std::max(best, -Exhaustive(child));
        }
    }
    return best;
}

TEST(Position, PlaysColumnsFromTheBottomAndKnowsWhenOneIsFull)
{
    Position position;
    EXPECT_EQ(position.moves(), 0);
    for (int count = 0; count != Position::kHeight; ++count)
    {
        ASSERT_TRUE(position.CanPlay(3));
        position.Play(3);
    }
    EXPECT_FALSE(position.CanPlay(3));
    EXPECT_FALSE(position.CanPlay(-1));
    EXPECT_FALSE(position.CanPlay(Position::kWidth));
    EXPECT_EQ(position.moves(), Position::kHeight);
    EXPECT_EQ(position.ToString(), "| | | |O| | | |\n"
                                   "| | | |X| | | |\n"
                                   "| | | |O| | | |\n"
                                   "| | | |X| | | |\n"
                                   "| | | |O| | | |\n"
                                   "| | | |X| | | |\n");
    EXPECT_EQ(CountBits(position.mask()), Position::kHeight);
}

TEST(Position, StopsReadingMovesAtOneItCannotPlayOrAfterAFour)
{
    int played = -1;
    Position::FromMoves(std::vector<int>{0, 0, 0, 0, 0, 0, 0, 1}, &played);
    EXPECT_EQ(played, 6);
    Position::FromMoves(std::vector<int>{2, 9}, &played);
    EXPECT_EQ(played, 1);
    // X four up column 0; the moves after it are not played.
    Position::FromMoves(std::vector<int>{0, 1, 0, 1, 0, 1, 0, 1}, &played);
    EXPECT_EQ(played, 7);
}

TEST(Position, SeesFourInEveryDirection)
{
    EXPECT_TRUE(After({0, 0, 1, 1, 2, 2}).IsWinningMove(3));             // across
    EXPECT_TRUE(After({4, 5, 4, 5, 4, 5}).IsWinningMove(4));             // up
    EXPECT_TRUE(After({0, 1, 1, 2, 2, 3, 2, 3, 3, 6}).IsWinningMove(3)); // the / diagonal
    EXPECT_TRUE(After({6, 5, 5, 4, 4, 3, 4, 3, 3, 0}).IsWinningMove(3)); // the \ diagonal
    EXPECT_FALSE(After({0, 0, 1, 1, 2, 2}).IsWinningMove(4));
    EXPECT_TRUE(After({0, 0, 1, 1, 2, 2}).CanWinNext());
    EXPECT_FALSE(Position().CanWinNext());
}

TEST(Position, KnowsTheMovesThatDoNotLoseAtOnce)
{
    // O to move against X's three across the bottom, open at 3: only the block.
    const Position block = After({0, 6, 1, 6, 2});
    EXPECT_EQ(block.PossibleNonLosingMoves(), Position::BottomMask(3));
    // X open at both ends (1 and 5 on the bottom, three in the middle): O cannot stop both.
    const Position lost = After({2, 2, 3, 3, 4});
    EXPECT_EQ(lost.PossibleNonLosingMoves(), 0u);
    // A slot under the other side's winning slot is left alone: X's three across row 1 wait on (3,1), so O will not
    // play (3,0).
    const Position under = After({0, 0, 1, 1, 2, 2, 6, 6});
    EXPECT_EQ(under.PossibleNonLosingMoves() & Position::BottomMask(3), 0u);
}

/**
 * Random games on a board of `P`'s size, played on the board (Game) and the position at once: the two must agree on
 * every four - two implementations of the rules, each the other's check.
 * @tparam P The position type.
 * @param games How many games.
 * @param seed The games' randomness.
 * @return How many ended in a four.
 */
template <typename P> int AgreeOnRandomGames(int games, unsigned seed)
{
    std::mt19937 random(seed);
    int          fours = 0;
    for (int game_number = 0; game_number != games; ++game_number)
    {
        Game game(P::kWidth, P::kHeight);
        P    position;
        int  player = kMin;
        while (game.HasRoom())
        {
            std::vector<int> open;
            for (int column = 0; column != P::kWidth; ++column)
            {
                if (game.IsMovePossible(column))
                {
                    open.push_back(column);
                }
            }
            const int column = open[std::uniform_int_distribution<std::size_t>(0, open.size() - 1)(random)];
            EXPECT_TRUE(position.CanPlay(column));
            const bool will_win = position.IsWinningMove(column);
            position.Play(column);
            game.DoMove(column, player);
            EXPECT_EQ(will_win, game.IsWinningMove(column, player)) << game.ToString();
            EXPECT_EQ(will_win, P::HasFour(position.current() ^ position.mask()));
            if (will_win)
            {
                ++fours;
                break;
            }
            player = -player;
        }
    }
    return fours;
}

TEST(Position, AgreesWithTheBoardOnEveryFourInRandomGames)
{
    EXPECT_GT((AgreeOnRandomGames<BasicPosition<6, 5, std::uint64_t>>(300, 1)), 200);
    EXPECT_GT((AgreeOnRandomGames<Position>(500, 2026)), 400);
    EXPECT_GT((AgreeOnRandomGames<BasicPosition<8, 7, std::uint64_t>>(300, 3)), 250);
    EXPECT_GT((AgreeOnRandomGames<BasicPosition<9, 7, Bits128>>(300, 4)), 250);
    EXPECT_GT((AgreeOnRandomGames<BasicPosition<16, 8, Bits192>>(200, 5)), 180);
}

TEST(WideBits, BehavesAsAWideUnsignedInteger)
{
    // A carry and a borrow across each word boundary.
    const Bits192 top_of_first{~std::uint64_t{0}};
    const Bits192 carried = top_of_first + Bits192{1};
    EXPECT_EQ(carried.words[0], 0u);
    EXPECT_EQ(carried.words[1], 1u);
    EXPECT_EQ(carried - Bits192{1}, top_of_first);
    Bits192 full_two     = top_of_first;
    full_two.words[1]    = ~std::uint64_t{0};
    const Bits192 rolled = full_two + Bits192{1};
    EXPECT_EQ(rolled.words[2], 1u);
    EXPECT_EQ(rolled.words[0] | rolled.words[1], 0u);
    EXPECT_EQ(rolled - Bits192{1}, full_two);
    // Shifts across the boundaries and back.
    const Bits192 one{1};
    EXPECT_EQ((one << 64).words[1], 1u);
    EXPECT_EQ((one << 130).words[2], 4u);
    EXPECT_EQ((one << 130) >> 130, one);
    EXPECT_EQ((Bits192{0b1011} << 62).words[0], std::uint64_t{0b11} << 62);
    EXPECT_EQ((Bits192{0b1011} << 62).words[1], 0b10u);
    EXPECT_EQ((Bits192{0b1011} << 62) >> 62, Bits192{0b1011});
    EXPECT_EQ(one << 0, one);
    EXPECT_EQ(one >> 0, one);
    // The bitwise operations, the count, the test for any.
    EXPECT_EQ(CountBits(~Bits192{}), 192);
    EXPECT_EQ(rolled & ~rolled, Bits192{});
    EXPECT_EQ(rolled ^ rolled, Bits192{});
    EXPECT_FALSE(Any(Bits192{}));
    EXPECT_TRUE(Any(one << 150));
    Bits192 built;
    built |= one;
    built ^= one << 1;
    built &= one << 1;
    EXPECT_EQ(built, one << 1);
    EXPECT_NE(HashBits(one << 1), HashBits(one));
    EXPECT_EQ(HashBits(std::uint64_t{77}), 77u);
}

TEST(Position, GivesEveryPositionItsOwnKey)
{
    std::mt19937            random(7);
    std::set<std::uint64_t> keys;
    std::set<std::string>   boards;
    for (int sample = 0; sample != 2000; ++sample)
    {
        Position  position;
        const int length = std::uniform_int_distribution<int>(0, 12)(random);
        for (int move = 0; move != length; ++move)
        {
            const int column = std::uniform_int_distribution<int>(0, Position::kWidth - 1)(random);
            if (position.CanPlay(column) && !position.IsWinningMove(column))
            {
                position.Play(column);
            }
        }
        const bool is_new_key   = keys.insert(position.Key()).second;
        const bool is_new_board = boards.insert(position.ToString() + std::to_string(position.moves() % 2)).second;
        ASSERT_EQ(is_new_key, is_new_board);
    }
}

TEST(Engine, SolvesLatePositionsAsExhaustiveSearchDoes)
{
    std::mt19937     random(42);
    Search<Position> engine(1);
    int              checked = 0;
    while (checked != 60)
    {
        // A random game stopped between 28 and 34 stones, no four yet and none to be had at once.
        Position  position;
        const int stones  = std::uniform_int_distribution<int>(28, 34)(random);
        bool      is_open = true;
        while (position.moves() < stones && is_open)
        {
            std::vector<int> open;
            for (int column = 0; column != Position::kWidth; ++column)
            {
                if (position.CanPlay(column) && !position.IsWinningMove(column))
                {
                    open.push_back(column);
                }
            }
            is_open = !open.empty();
            if (is_open)
            {
                position.Play(open[std::uniform_int_distribution<std::size_t>(0, open.size() - 1)(random)]);
            }
        }
        if (!is_open || position.moves() < stones)
        {
            continue;
        }
        const std::optional<int> solved = engine.Solve(position, 30.0);
        ASSERT_TRUE(solved.has_value());
        ASSERT_EQ(*solved, Exhaustive(position)) << position.ToString();
        ++checked;
    }
}

TEST(Engine, ValuesSoonerWinsMore)
{
    // X to move can win at once at 3; every other move lets O play on. Perfect values the win highest.
    Search<Position>             engine(3);
    const Position               position = After({0, 0, 1, 1, 2, 2});
    const std::vector<MoveValue> values   = engine.Values(position, Strength::kPerfect, 30.0);
    const auto best = std::max_element(values.begin(), values.end(),
                                       [](const MoveValue &a, const MoveValue &b) { return a.score < b.score; });
    EXPECT_EQ(best->column, 3);
    EXPECT_TRUE(best->is_exact);
    EXPECT_EQ(best->score, (Position::kSlots + 1 - 6) / 2 * kWinScale);
    for (const Strength strength : {Strength::kEasy, Strength::kMedium, Strength::kHard, Strength::kPerfect})
    {
        EXPECT_EQ(engine.ChooseMove(position, strength, nullptr, 5.0), 3);
    }
}

TEST(Engine, StopsTheMoveItIsAsked)
{
    // From the empty board an exact solution takes far longer than this: stopped, the solver has no answer.
    Search<Position>         engine(5);
    std::atomic<bool>        stop{false};
    std::thread              stopper([&stop] {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        stop.store(true);
    });
    const auto               start  = std::chrono::steady_clock::now();
    const std::optional<int> solved = engine.Solve(Position(), 600.0, &stop);
    stopper.join();
    EXPECT_FALSE(solved.has_value());
    EXPECT_LT(std::chrono::steady_clock::now() - start, std::chrono::seconds(5));
    // Out of time, a level still answers with a move it can play.
    EXPECT_TRUE(Position().CanPlay(engine.ChooseMove(Position(), Strength::kPerfect, nullptr, 0.05)));
    EXPECT_GT(engine.nodes(), 0u);
}

TEST(Engine, JudgesThreatsAndTheCentre)
{
    EXPECT_EQ(Search<Position>::Judge(Position()), 0);
    // X in the centre, O to move: the judgement is from O's side, behind.
    EXPECT_LT(Search<Position>::Judge(After({3})), 0);
    // X three across the bottom with both ends open, O to move: far behind, but still a judgement, not a loss.
    const int judged = Search<Position>::Judge(After({2, 6, 3, 6, 4}));
    EXPECT_LT(judged, 0);
    EXPECT_GT(judged, -kWinScale);
}

/**
 * A game between two players to its end.
 * @param first Chooses the first player's moves.
 * @param second Chooses the second's.
 * @return 1 when the first wins, -1 the second, 0 a draw.
 */
template <typename First, typename Second> int PlayGame(First first, Second second)
{
    Position position;
    for (int turn = 0; turn != Position::kSlots; ++turn)
    {
        const int column = turn % 2 == 0 ? first(position) : second(position);
        EXPECT_TRUE(position.CanPlay(column));
        if (position.IsWinningMove(column))
        {
            return turn % 2 == 0 ? 1 : -1;
        }
        position.Play(column);
    }
    return 0;
}

/**
 * Games between two players, each starting half of them.
 * @param games How many.
 * @param one The first player.
 * @param other The second.
 * @return `one`'s wins less its losses.
 */
template <typename One, typename Other> int Match(int games, One one, Other other)
{
    int balance = 0;
    for (int game = 0; game != games; ++game)
    {
        balance += game % 2 == 0 ? PlayGame(one, other) : -PlayGame(other, one);
    }
    return balance;
}

TEST(Engine, LevelsBeatTheLevelsBelowThem)
{
    std::mt19937 random(11);
    const auto   aimless = [&random](const Position &position) {
        std::vector<int> open;
        for (int column = 0; column != Position::kWidth; ++column)
        {
            if (position.CanPlay(column))
            {
                open.push_back(column);
            }
        }
        return open[std::uniform_int_distribution<std::size_t>(0, open.size() - 1)(random)];
    };
    Search<Position> easy_engine(21);
    Search<Position> medium_engine(22);
    Search<Position> hard_engine(23);
    const auto       easy = [&](const Position &position) { return easy_engine.ChooseMove(position, Strength::kEasy); };
    const auto medium = [&](const Position &position) { return medium_engine.ChooseMove(position, Strength::kMedium); };
    const auto hard   = [&](const Position &position) {
        return hard_engine.ChooseMove(position, Strength::kHard, nullptr, 0.2);
    };
    EXPECT_GT(Match(20, easy, aimless), 10); // Easy beats a player choosing at random
    EXPECT_GT(Match(20, medium, easy), 10);  // Medium beats Easy
    EXPECT_GT(Match(10, hard, medium), 3);   // Hard beats Medium
}

TEST(Engine, VariesItsGames)
{
    // Two engines with different seeds, each playing itself at Medium: the games differ.
    Search<Position> first(31);
    Search<Position> second(32);
    const auto       play = [](Search<Position> &engine) {
        std::vector<int> moves;
        Position         position;
        for (int turn = 0; turn != 8; ++turn)
        {
            const int column = engine.ChooseMove(position, Strength::kMedium);
            moves.push_back(column);
            if (position.IsWinningMove(column))
            {
                break;
            }
            position.Play(column);
        }
        return moves;
    };
    EXPECT_NE(play(first), play(second));
}

TEST(Search, SolvesTheSmallBoardsLatePositionsAsExhaustiveSearchDoes)
{
    using Small = BasicPosition<6, 5, std::uint64_t>;
    std::mt19937  random(43);
    Search<Small> search(2);
    int           checked = 0;
    while (checked != 30)
    {
        Small     position;
        const int stones  = std::uniform_int_distribution<int>(16, 22)(random);
        bool      is_open = true;
        while (position.moves() < stones && is_open)
        {
            std::vector<int> open;
            for (int column = 0; column != Small::kWidth; ++column)
            {
                if (position.CanPlay(column) && !position.IsWinningMove(column))
                {
                    open.push_back(column);
                }
            }
            is_open = !open.empty();
            if (is_open)
            {
                position.Play(open[std::uniform_int_distribution<std::size_t>(0, open.size() - 1)(random)]);
            }
        }
        if (!is_open)
        {
            continue;
        }
        const std::optional<int> solved = search.Solve(position, 30.0);
        ASSERT_TRUE(solved.has_value());
        ASSERT_EQ(*solved, Exhaustive(position)) << position.ToString();
        ++checked;
    }
}

TEST(Engine, PlaysOnEveryBoardSizeItOffers)
{
    for (const BoardSize size : kBoardSizes)
    {
        EXPECT_TRUE(IsOffered(size));
        Engine engine(size, 9);
        EXPECT_EQ(engine.size(), size);
        // A few moves into the game, every level answers with a column that has room, quickly.
        const std::vector<int> moves{size.columns / 2, size.columns / 2, 0};
        for (const Strength strength : {Strength::kEasy, Strength::kMedium, Strength::kHard, Strength::kPerfect})
        {
            const int column = engine.ChooseMove(moves, strength, nullptr, 0.3);
            EXPECT_GE(column, 0);
            EXPECT_LT(column, size.columns);
        }
        const std::vector<MoveValue> values = engine.Values(moves, Strength::kMedium, 0.3);
        EXPECT_EQ(values.size(), static_cast<std::size_t>(size.columns));
    }
    // A size not offered is the standard board.
    EXPECT_FALSE(IsOffered({5, 5}));
    EXPECT_EQ(Engine(BoardSize{5, 5}, 1).size(), kStandardBoard);
}

TEST(Engine, SolvesThroughItsInterface)
{
    Engine                   engine(kStandardBoard, 4);
    const std::vector<int>   moves{0, 0, 1, 1, 2, 2};
    const std::optional<int> solved = engine.Solve(moves, 10.0);
    ASSERT_TRUE(solved.has_value());
    EXPECT_EQ(*solved, (Position::kSlots + 1 - 6) / 2); // X wins at once
    std::atomic<bool> stop{true};
    EXPECT_FALSE(engine.Solve(std::vector<int>{}, 10.0, &stop).has_value());
}

} // namespace
} // namespace knect4
