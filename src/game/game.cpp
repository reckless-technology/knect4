#include "game/game.h"

#include <algorithm>
#include <cassert>

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
    // An empty column holds no move to have won with; EvaluateMove would read a line through the row below the board.
    if (column < 0 || column >= width_ || ColumnHeight(column) == 0)
    {
        return false;
    }
    return EvaluateMove(column, true, player) >= kWinningScore;
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

int Game::EvaluateMove(int column, bool has_move_been_made, int player) const
{
    const int row   = has_move_been_made ? ColumnHeight(column) - 1 : ColumnHeight(column);
    int       score = 0;
    assert(row >= 0 && row < height_); // a piece there, or a slot it could drop into

    // Vertically only the pieces below count: nothing stands above a column's top piece.
    int length = 1;
    for (int y = row - 1; y >= 0 && At(column, y) == player; --y)
    {
        ++length;
    }
    score += kScores[static_cast<std::size_t>(std::min(length, 4))];

    // Horizontally, then the \ diagonal (up to the left, down to the right) and the / diagonal.
    score += kScores[static_cast<std::size_t>(std::min(LineLength(column, row, 1, 0, player), 4))];
    score += kScores[static_cast<std::size_t>(std::min(LineLength(column, row, -1, 1, player), 4))];
    score += kScores[static_cast<std::size_t>(std::min(LineLength(column, row, 1, 1, player), 4))];
    return score;
}

int Game::EvaluateBoard() const
{
    int max_best_score = 0;
    int min_best_score = 0;
    for (int column = 0; column != width_; ++column)
    {
        if (ColumnHeight(column) != 0)
        {
            const int player = At(column, ColumnHeight(column) - 1);
            const int score  = EvaluateMove(column, true, player);
            if (player == kMax)
            {
                max_best_score = std::max(max_best_score, score);
            }
            else
            {
                min_best_score = std::max(min_best_score, score);
            }
        }
    }
    return max_best_score - min_best_score;
}

int Game::Negamax(int depth, int alpha, int beta, bool do_alpha_beta, int player, int &best_move,
                  const std::atomic<bool> *stop)
{
    assert(depth >= 0);
    if (stop != nullptr && stop->load(std::memory_order_relaxed))
    {
        return 0;
    }
    std::array<Move, kMostColumns> moves{};
    int                            count = 0;
    for (int column = 0; column != width_; ++column)
    {
        if (IsMovePossible(column))
        {
            const int score = EvaluateMove(column, false, player);
            if (score >= kWinningScore)
            {
                // A win: the node ends here.
                best_move = column;
                return score;
            }
            moves[static_cast<std::size_t>(count)] = {column, score};
            ++count;
        }
    }
    if (count == 0)
    {
        // A full board: a draw.
        return 0;
    }
    SortBestFirst(moves.data(), count);
    int below = 0; // the nodes below report a best move no one reads
    for (int index = 0; index != count; ++index)
    {
        Move &move = moves[static_cast<std::size_t>(index)];
        DoMove(move.column, player);
        if (depth != 0)
        {
            move.score = do_alpha_beta ? -Negamax(depth - 1, -beta, -alpha, true, -player, below, stop)
                                       : -Negamax(depth - 1, kAlphaInitial, kBetaInitial, false, -player, below, stop);
        }
        else
        {
            // A leaf of an open game, scored from the side to move.
            move.score = player == kMax ? EvaluateBoard() : -EvaluateBoard();
        }
        UndoMove(move.column);
        if (move.score > alpha)
        {
            alpha     = move.score;
            best_move = move.column;
        }
        if (do_alpha_beta && move.score >= beta)
        {
            // Higher up, the other side already has a line that keeps this one from happening.
            return beta;
        }
    }
    return alpha;
}

void Game::SortBestFirst(Move *moves, int count)
{
    // The .NET Framework 4.8's List.Sort on a list of at most sixteen: a list of three by three exchanges (which may
    // reorder equal scores), any other by insertion (which keeps them in column order). Move's comparison put the
    // higher score first.
    const auto exchange_if_better_later = [moves](int first, int second) {
        if (moves[second].score > moves[first].score)
        {
            std::swap(moves[first], moves[second]);
        }
    };
    if (count == 3)
    {
        exchange_if_better_later(0, 1);
        exchange_if_better_later(0, 2);
        exchange_if_better_later(1, 2);
        return;
    }
    for (int index = 1; index < count; ++index)
    {
        const Move move  = moves[index];
        int        place = index;
        while (place > 0 && move.score > moves[place - 1].score)
        {
            moves[place] = moves[place - 1];
            --place;
        }
        moves[place] = move;
    }
}

int Game::NextMove(int depth, int player, int &best_move, const std::atomic<bool> *stop)
{
    return Negamax(depth, kAlphaInitial, kBetaInitial, true, player, best_move, stop);
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
