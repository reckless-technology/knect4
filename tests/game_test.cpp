#include "game/game.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace connect4
{
namespace
{

/**
 * Pieces played in turn, X (kMax) first.
 * @param columns The columns, in order.
 * @return The game.
 */
Game Played(const std::vector<int> &columns)
{
    Game game;
    int  player = kMax;
    for (const int column : columns)
    {
        game.DoMove(column, player);
        player = -player;
    }
    return game;
}

TEST(Game, StartsEmptyAndTakesSixPiecesAColumn)
{
    Game game;
    EXPECT_EQ(game.width(), 7);
    EXPECT_EQ(game.height(), 6);
    for (int count = 0; count != 6; ++count)
    {
        EXPECT_TRUE(game.IsMovePossible(3));
        game.DoMove(3, count % 2 == 0 ? kMax : kMin);
    }
    EXPECT_FALSE(game.IsMovePossible(3));
    EXPECT_FALSE(game.IsMovePossible(-1));
    EXPECT_FALSE(game.IsMovePossible(7));
    EXPECT_EQ(game.ColumnHeight(3), 6);
    EXPECT_EQ(game.At(3, 0), kMax);
    EXPECT_EQ(game.At(3, 5), kMin);
    game.UndoMove(3);
    EXPECT_EQ(game.ColumnHeight(3), 5);
    EXPECT_EQ(game.At(3, 5), kEmpty);
    game.Reset();
    EXPECT_EQ(game.ColumnHeight(3), 0);
    EXPECT_EQ(game.At(3, 0), kEmpty);
}

TEST(Game, WritesItselfTopRowFirst)
{
    const Game game = Played({3, 3});
    EXPECT_EQ(game.ToString(), "| | | | | | | |\n"
                               "| | | | | | | |\n"
                               "| | | | | | | |\n"
                               "| | | | | | | |\n"
                               "| | | |O| | | |\n"
                               "| | | |X| | | |\n"
                               "_______________\n");
}

TEST(Game, ScoresAMoveByItsLinesInFourDirections)
{
    Game game;
    // A lone piece: a line of one each way.
    EXPECT_EQ(game.EvaluateMove(3, false, kMax), 4 * Game::kScores[1]);
    game.DoMove(3, kMax);
    EXPECT_EQ(game.EvaluateMove(3, true, kMax), 4 * Game::kScores[1]);
    // Beside it: two in a row across.
    EXPECT_EQ(game.EvaluateMove(4, false, kMax), Game::kScores[2] + 3 * Game::kScores[1]);
    // On top of it: two in a column.
    EXPECT_EQ(game.EvaluateMove(3, false, kMax), Game::kScores[2] + 3 * Game::kScores[1]);
    // The other side's piece there makes no line with it.
    EXPECT_EQ(game.EvaluateMove(4, false, kMin), 4 * Game::kScores[1]);
}

TEST(Game, EvaluatesTheBoardFromTheComputersSide)
{
    Game game;
    EXPECT_EQ(game.EvaluateBoard(), 0);
    game.DoMove(0, kMax);
    game.DoMove(1, kMax);
    game.DoMove(6, kMin);
    // X's best top piece is two across (13 + 3), O's a lone one (4).
    EXPECT_EQ(game.EvaluateBoard(), 16 - 4);
}

TEST(Game, FindsFourInEveryDirection)
{
    // Across: X on the bottom row, O on top of each.
    Game across = Played({0, 0, 1, 1, 2, 2, 3});
    EXPECT_TRUE(across.IsWinningMove(3, kMax));
    EXPECT_FALSE(across.IsDraw(3, kMax));

    // Up a column.
    Game up = Played({4, 5, 4, 5, 4, 5, 4});
    EXPECT_TRUE(up.IsWinningMove(4, kMax));
    EXPECT_FALSE(up.IsWinningMove(5, kMin));

    // The / diagonal, from (0,0) to (3,3).
    Game rising = Played({0, 1, 1, 2, 2, 3, 2, 3, 3, 6, 3});
    EXPECT_TRUE(rising.IsWinningMove(3, kMax));

    // The \ diagonal, from (6,0) to (3,3).
    Game falling = Played({6, 5, 5, 4, 4, 3, 4, 3, 3, 0, 3});
    EXPECT_TRUE(falling.IsWinningMove(3, kMax));
}

TEST(Game, LightsTheWinningLineAndOnlyIt)
{
    Game              game = Played({0, 0, 1, 1, 2, 2, 3});
    std::vector<Cell> line = game.WinningLine(3);
    std::sort(line.begin(), line.end(), [](Cell a, Cell b) { return a.column < b.column; });
    EXPECT_EQ(line, (std::vector<Cell>{{0, 0}, {1, 0}, {2, 0}, {3, 0}}));

    // A piece that closes two lines at once lights both, its own slot once: column 3 X three high, O under columns 0
    // to 2 and X on them in row 3, then X on top of column 3.
    Game both;
    for (int row = 0; row != 3; ++row)
    {
        both.DoMove(3, kMax);
        for (int column = 0; column != 3; ++column)
        {
            both.DoMove(column, kMin);
        }
    }
    for (int column = 0; column != 3; ++column)
    {
        both.DoMove(column, kMax);
    }
    both.DoMove(3, kMax);
    EXPECT_TRUE(both.IsWinningMove(3, kMax));
    EXPECT_EQ(both.WinningLine(3).size(), 7u);

    // No line, nothing lit; an empty column, nothing.
    EXPECT_TRUE(Played({0, 1}).WinningLine(1).empty());
    EXPECT_TRUE(Game().WinningLine(2).empty());
}

TEST(Game, CallsAFullBoardWithNoFourADraw)
{
    // Each column X X O O X X from the bottom, but the middle one O O X X O O: no four anywhere.
    Game game;
    int  player = kMax;
    int  last   = 0;
    for (const int column : {0, 1, 2, 3, 4, 5, 6})
    {
        for (int row = 0; row != 6; ++row)
        {
            const int piece = ((row / 2) % 2 == 0) == (column != 3) ? kMax : kMin;
            game.DoMove(column, piece);
            last   = column;
            player = piece;
        }
    }
    EXPECT_FALSE(game.HasRoom());
    EXPECT_FALSE(game.IsWinningMove(last, player));
    EXPECT_TRUE(game.IsDraw(last, player));
}

TEST(Game, TakesAWinAndBlocksOne)
{
    // X has three across the bottom with the fourth open: X takes it at once.
    Game win  = Played({0, 0, 1, 1, 2, 2});
    int  move = -1;
    EXPECT_GE(win.NextMove(3, kMax, move), Game::kWinningScore);
    EXPECT_EQ(move, 3);

    // O to move against the same three: it blocks.
    Game block = Played({0, 6, 1, 6, 2});
    move       = -1;
    block.NextMove(3, kMin, move);
    EXPECT_EQ(move, 3);
}

TEST(Game, PrunesToTheSameMoveAndScoreAsTheFullSearch)
{
    // The original's own check (DoSelfTests): alpha-beta against plain negamax, both sides, through a whole game.
    Game game;
    int  player = kMax;
    for (int ply = 0; ply != 42; ++ply)
    {
        int       plain_move  = -1;
        int       pruned_move = -1;
        const int plain       = game.Negamax(4, Game::kAlphaInitial, Game::kBetaInitial, false, player, plain_move);
        const int pruned      = game.Negamax(4, Game::kAlphaInitial, Game::kBetaInitial, true, player, pruned_move);
        ASSERT_EQ(plain, pruned) << "ply " << ply << "\n" << game.ToString();
        ASSERT_EQ(plain_move, pruned_move) << "ply " << ply << "\n" << game.ToString();
        game.DoMove(pruned_move, player);
        if (game.IsWinningMove(pruned_move, player) || game.IsDraw(pruned_move, player))
        {
            break;
        }
        player = -player;
    }
}

TEST(Game, PlaysAsTheOriginalPlayed)
{
    // Self-play recorded from the C# game: every move and score, at depths 1 to 7, from openings of a few moves.
    std::ifstream file(CONNECT4_TEST_DATA "/csharp_self_play.txt");
    ASSERT_TRUE(file) << "the fixture " CONNECT4_TEST_DATA "/csharp_self_play.txt";
    std::string line;
    int         games = 0;
    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#')
        {
            continue;
        }
        const std::size_t colon = line.find(':');
        ASSERT_NE(colon, std::string::npos) << line;
        std::istringstream head(line.substr(0, colon));
        int                depth = 0;
        std::string        opening;
        head >> depth >> opening;
        Game game;
        int  player = kMax;
        for (const char column : opening)
        {
            game.DoMove(column - '0', player);
            player = -player;
        }
        std::istringstream moves(line.substr(colon + 1));
        std::string        word;
        std::string        result;
        while (moves >> word)
        {
            const std::size_t slash = word.find('/');
            if (slash == std::string::npos)
            {
                result = word;
                break;
            }
            const int expected_move  = std::stoi(word.substr(0, slash));
            const int expected_score = std::stoi(word.substr(slash + 1));
            int       move           = -1;
            const int score          = game.NextMove(depth, player, move);
            ASSERT_EQ(move, expected_move) << "depth " << depth << " opening '" << opening << "'\n" << game.ToString();
            ASSERT_EQ(score, expected_score) << "depth " << depth << " opening '" << opening << "'\n"
                                             << game.ToString();
            game.DoMove(move, player);
            const bool is_won   = game.IsWinningMove(move, player);
            const bool is_drawn = game.IsDraw(move, player);
            if (is_won || is_drawn)
            {
                ASSERT_TRUE(moves >> result);
                EXPECT_EQ(result, is_drawn ? "D" : (player == kMax ? "X" : "O")) << line;
                break;
            }
            player = -player;
        }
        ++games;
    }
    EXPECT_EQ(games, 49);
}

TEST(Game, StopsWhenAsked)
{
    Game              game;
    std::atomic<bool> stop{true};
    int               move  = -1;
    const auto        start = std::chrono::steady_clock::now();
    EXPECT_EQ(game.NextMove(11, kMax, move, &stop), 0);
    EXPECT_LT(std::chrono::steady_clock::now() - start, std::chrono::seconds(1));
    EXPECT_EQ(move, -1);
    // The board is as it was.
    EXPECT_EQ(game.ToString(), Game().ToString());
}

} // namespace
} // namespace connect4
