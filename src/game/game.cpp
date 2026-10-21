#include "game/game.h"

#include <algorithm>
#include <cassert>
#include <utility>

namespace knect4
{

Game::Game(int width, int height)
    : width_(std::clamp(width, 1, kMostColumns)), height_(std::max(height, 1)),
      board_(static_cast<std::size_t>(width_ * height_), kEmpty), column_height_(static_cast<std::size_t>(width_), 0)
{
}

void Game::Reset()
{
    std::fill(board_.begin(), board_.end(), kEmpty);
    std::fill(column_height_.begin(), column_height_.end(), 0);
}

bool Game::IsMovePossible(int column) const
{
    return column >= 0 && column < width_ && column_height_[static_cast<std::size_t>(column)] != height_;
}

void Game::DoMove(int column, int player)
{
    // A column with no room, or off the board, is a caller's mistake: refused rather than written past the board.
    assert(IsMovePossible(column));
    if (!IsMovePossible(column))
    {
        return;
    }
    int &column_height                                             = column_height_[static_cast<std::size_t>(column)];
    board_[static_cast<std::size_t>(Index(column, column_height))] = player;
    ++column_height;
}

void Game::UndoMove(int column)
{
    assert(column >= 0 && column < width_ && column_height_[static_cast<std::size_t>(column)] > 0);
    if (column < 0 || column >= width_ || column_height_[static_cast<std::size_t>(column)] == 0)
    {
        return;
    }
    int &column_height = column_height_[static_cast<std::size_t>(column)];
    --column_height;
    board_[static_cast<std::size_t>(Index(column, column_height))] = kEmpty;
}

int Game::At(int column, int row) const
{
    return board_[static_cast<std::size_t>(Index(column, row))];
}

int Game::ColumnHeight(int column) const
{
    return column_height_[static_cast<std::size_t>(column)];
}

bool Game::HasRoom() const
{
    for (int column = 0; column != width_; ++column)
    {
        if (IsMovePossible(column))
        {
            return true;
        }
    }
    return false;
}

bool Game::IsWinningMove(int column, int player) const
{
    // An empty column holds no move to have won with.
    if (column < 0 || column >= width_ || ColumnHeight(column) == 0)
    {
        return false;
    }
    const int row = ColumnHeight(column) - 1;
    if (At(column, row) != player)
    {
        return false;
    }
    // Up (only the pieces below count: nothing stands above the top piece), across and the two diagonals.
    for (const auto &[step_column, step_row] : {std::pair{0, 1}, std::pair{1, 0}, std::pair{-1, 1}, std::pair{1, 1}})
    {
        if (LineLength(column, row, step_column, step_row, player) >= 4)
        {
            return true;
        }
    }
    return false;
}

bool Game::IsDraw(int column, int player) const
{
    return !HasRoom() && !IsWinningMove(column, player);
}

std::vector<Cell> Game::WinningLine(int column) const
{
    std::vector<Cell> cells;
    const int         row = ColumnHeight(column) - 1;
    if (row < 0)
    {
        return cells;
    }
    const int player = At(column, row);
    for (const auto &[step_column, step_row] : {std::pair{0, 1}, std::pair{1, 0}, std::pair{-1, 1}, std::pair{1, 1}})
    {
        if (LineLength(column, row, step_column, step_row, player) < 4)
        {
            continue;
        }
        for (const int sign : {1, -1})
        {
            int x = column;
            int y = row;
            while (x >= 0 && x < width_ && y >= 0 && y < height_ && At(x, y) == player)
            {
                if (std::find(cells.begin(), cells.end(), Cell{x, y}) == cells.end())
                {
                    cells.push_back({x, y});
                }
                x += sign * step_column;
                y += sign * step_row;
            }
        }
    }
    return cells;
}

int Game::LineLength(int column, int row, int step_column, int step_row, int player) const
{
    int length = 1;
    for (const int sign : {1, -1})
    {
        int x = column + sign * step_column;
        int y = row + sign * step_row;
        while (x >= 0 && x < width_ && y >= 0 && y < height_ && At(x, y) == player)
        {
            x += sign * step_column;
            y += sign * step_row;
            ++length;
        }
    }
    return length;
}

std::string Game::ToString() const
{
    std::string text;
    for (int row = height_ - 1; row >= 0; --row)
    {
        text += '|';
        for (int column = 0; column != width_; ++column)
        {
            const int piece = At(column, row);
            text += piece == kMax ? 'X' : (piece == kMin ? 'O' : ' ');
            text += '|';
        }
        text += '\n';
    }
    text.append(static_cast<std::size_t>(2 * width_ + 1), '_');
    text += '\n';
    return text;
}

} // namespace knect4
