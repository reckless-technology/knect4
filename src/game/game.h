#pragma once
/// @file game.h The board, its moves and the computer's search: the C# game's Game.cs (2014) in C++, its board, its
/// evaluation and its negamax search kept as they were.

#include <array>
#include <atomic>
#include <limits>
#include <string>
#include <vector>

namespace knect4
{

inline constexpr int kMax   = 1;  ///< The computer's pieces, the side the evaluation is written for.
inline constexpr int kMin   = -1; ///< The person's pieces.
inline constexpr int kEmpty = 0;  ///< An empty slot.

/** A slot on the board: its column from the left and its row from the bottom, both from 0. */
struct Cell
{
    int column{0}; ///< The column, 0 at the left.
    int row{0};    ///< The row, 0 at the bottom.

    /**
     * @param other The other slot.
     * @return Whether the two are the same slot.
     */
    bool operator==(const Cell &other) const = default;
};

/**
 * A game of four in a row: a board of columns the pieces drop down, each player's pieces `kMax` or `kMin`, and the
 * computer's search for its next move.
 *
 * The search is the original's: negamax to a fixed depth with alpha-beta pruning, the moves at each node tried best
 * first by the score their own line lengths give (EvaluateMove), a winning move ending the search at once with its
 * score, a full board scored 0, and a leaf at the depth scored by EvaluateBoard from the side to move. Moves of equal
 * score are ordered as the original's sort (the .NET Framework 4.8's List.Sort) left them, so the search chooses among
 * equally good moves as the original did and plays its games move for move (tests/data/csharp_self_play.txt).
 */
class Game
{
  public:
    static constexpr int kDefaultWidth  = 7;  ///< The board's columns.
    static constexpr int kDefaultHeight = 6;  ///< Its rows.
    static constexpr int kMostColumns   = 16; ///< The widest board the search takes.

    /** What a run of n pieces in a line is worth, n from 0 to 4; the ratio of the 3 to the 2 shapes the play. */
    static constexpr std::array<int, 5> kScores{0, 1, 13, 37, 104729};
    static constexpr int                kWinningScore = kScores[4]; ///< Four in a row, or more.

    /** The window's lower edge: one above the least int, so that its negation is the window's upper edge. */
    static constexpr int kAlphaInitial = std::numeric_limits<int>::min() + 1;
    static constexpr int kBetaInitial  = -kAlphaInitial; ///< The window's upper edge.

    /**
     * An empty board.
     * @param width The columns, 1 to kMostColumns.
     * @param height The rows, at least 1.
     */
    explicit Game(int width = kDefaultWidth, int height = kDefaultHeight);

    /** Every slot emptied. */
    void Reset();

    /** @return The board's columns. */
    int width() const noexcept
    {
        return width_;
    }

    /** @return The board's rows. */
    int height() const noexcept
    {
        return height_;
    }

    /**
     * @param column A column on the board.
     * @return Whether a piece can still drop into it.
     */
    bool IsMovePossible(int column) const;

    /**
     * A piece of `player`'s dropped into `column`, which has room.
     * @param column The column.
     * @param player kMax or kMin.
     */
    void DoMove(int column, int player);

    /**
     * The top piece of `column`, which holds one, taken back.
     * @param column The column.
     */
    void UndoMove(int column);

    /**
     * @param column The column.
     * @param row The row.
     * @return What is in the slot: kMax, kMin or kEmpty.
     */
    int At(int column, int row) const;

    /**
     * @param column The column.
     * @return How many pieces it holds.
     */
    int ColumnHeight(int column) const;

    /** @return Whether any column has room. */
    bool HasRoom() const;

    /**
     * Whether the piece `player` has just dropped into `column` made four in a row.
     * @param column The column of the piece just played.
     * @param player Whose piece it is.
     * @return Whether it won.
     */
    bool IsWinningMove(int column, int player) const;

    /**
     * Whether the piece `player` has just dropped into `column` filled the board without winning.
     * @param column The column of the piece just played.
     * @param player Whose piece it is.
     * @return Whether the game is drawn.
     */
    bool IsDraw(int column, int player) const;

    /**
     * The slots of every line of four or more through the top piece of `column`, that piece included, for the window to
     * light; empty when it made none.
     * @param column A column holding a piece.
     * @return The slots, each once.
     */
    std::vector<Cell> WinningLine(int column) const;

    /**
     * What a piece of `player`'s in `column` is worth: the score (kScores) of the line it stands in, summed over the
     * four directions, each line counted to at most four.
     * @param column The column.
     * @param has_move_been_made True when the piece is already the column's top piece; false for the slot it would
     *        drop into, which must be on the board.
     * @param player Whose piece.
     * @return The score; kWinningScore or more for a four.
     */
    int EvaluateMove(int column, bool has_move_been_made, int player) const;

    /**
     * The board from kMax's side: the best EvaluateMove of the columns' top pieces that are kMax's, less the best of
     * those that are kMin's.
     * @return The score.
     */
    int EvaluateBoard() const;

    /**
     * Negamax from `player`'s side, `depth` plies beyond this one.
     * @param depth The plies left; 0 scores each move by EvaluateBoard.
     * @param alpha The window's lower edge.
     * @param beta Its upper edge.
     * @param do_alpha_beta False to search every move, with no pruning (the same answer, slower).
     * @param player The side to move.
     * @param best_move Set to the best move's column when one raises alpha; else left as it was.
     * @param stop Read at each node: once true, every node returns 0 and the answer means nothing; may be null.
     * @return The score from `player`'s side.
     */
    int Negamax(int depth, int alpha, int beta, bool do_alpha_beta, int player, int &best_move,
                const std::atomic<bool> *stop = nullptr);

    /**
     * The search the computer moves by: Negamax with the full window and pruning.
     * @param depth The plies beyond the move: Easy 3, Medium 7, Hard 11.
     * @param player The side to move.
     * @param best_move Set to the column to play.
     * @param stop Read at each node: once true the search ends and its answer means nothing; may be null.
     * @return The move's score from `player`'s side.
     */
    int NextMove(int depth, int player, int &best_move, const std::atomic<bool> *stop = nullptr);

    /** @return The board as text, the top row first: `X` for kMax, `O` for kMin, a space for an empty slot. */
    std::string ToString() const;

  private:
    /** A move at a node: its column and its score, first from EvaluateMove and then from the search below it. */
    struct Move
    {
        int column{0}; ///< The column.
        int score{0};  ///< Its score.
    };

    /**
     * A node's moves put best first, in the order the original's sort left them, equal scores included.
     * @param moves The moves, in column order.
     * @param count How many; at most kMostColumns.
     */
    static void SortBestFirst(Move *moves, int count);

    /**
     * @param column The column.
     * @param row The row.
     * @return Where the slot's value is kept.
     */
    int Index(int column, int row) const noexcept
    {
        return column * height_ + row;
    }

    /**
     * How many of `player`'s pieces stand in a line through (column, row), the slot itself counted as one of them,
     * stepping (step_column, step_row) one way and the opposite way.
     * @param column The slot's column.
     * @param row Its row.
     * @param step_column The step's columns.
     * @param step_row The step's rows.
     * @param player Whose pieces.
     * @return The line's length.
     */
    int LineLength(int column, int row, int step_column, int step_row, int player) const;

    int              width_;         ///< The columns.
    int              height_;        ///< The rows.
    std::vector<int> board_;         ///< Each slot, column by column from the bottom: kMax, kMin or kEmpty.
    std::vector<int> column_height_; ///< The pieces in each column.
};

} // namespace knect4
