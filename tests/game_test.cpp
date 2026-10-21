#include "game/game.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <vector>

namespace knect4
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

TEST(Game, RefusesMovesTheBoardHasNoRoomFor)
{
#ifdef NDEBUG
    // A release build refuses them rather than writing past the board; a debug build asserts.
    Game game = Played({0, 0, 0, 0, 0, 0});
    game.DoMove(0, kMax);
    game.DoMove(-1, kMax);
    game.DoMove(7, kMax);
    EXPECT_EQ(game.ColumnHeight(0), 6);
    Game empty;
    empty.UndoMove(3);
    empty.UndoMove(-1);
    empty.UndoMove(7);
    EXPECT_EQ(empty.ToString(), Game().ToString());
#else
    GTEST_SKIP() << "a debug build asserts on these instead";
#endif
}

TEST(Game, FindsNoWinInAColumnWithNoPiece)
{
    // X three up the diagonal to (2,0) from the left, column 3 empty: no piece there, so no win, whatever lies beside.
    Game game;
    game.DoMove(0, kMin);
    game.DoMove(0, kMin);
    game.DoMove(0, kMax); // (0,2)
    game.DoMove(1, kMin);
    game.DoMove(1, kMax); // (1,1)
    game.DoMove(2, kMax); // (2,0)
    EXPECT_FALSE(game.IsWinningMove(3, kMax));
    EXPECT_FALSE(game.IsWinningMove(-1, kMax));
    EXPECT_FALSE(game.IsWinningMove(7, kMax));
}

} // namespace
} // namespace knect4
