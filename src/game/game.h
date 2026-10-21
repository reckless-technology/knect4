#pragma once
/// @file game.h The board as the window shows it and the rules it is played by: the pieces in each slot, the moves,
/// four in a row and a draw. The computer's search plays on a Position (position.h, engine.h).

#include <string>
#include <vector>

namespace knect4
{

inline constexpr int kMax   = 1;  ///< The computer's pieces against the person, or Yellow's between two people.
inline constexpr int kMin   = -1; ///< The person's pieces, or Red's.
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

/** A game of four in a row: a board of columns the pieces drop down, each player's pieces `kMax` or `kMin`. */
class Game
{
  public:
    static constexpr int kDefaultWidth  = 7;  ///< The board's columns.
    static constexpr int kDefaultHeight = 6;  ///< Its rows.
    static constexpr int kMostColumns   = 16; ///< The widest board taken.

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
     * @param column A column.
     * @return Whether a piece can still drop into it: on the board and with room.
     */
    bool IsMovePossible(int column) const;

    /**
     * A piece of `player`'s dropped into `column`, which must have room; a column off the board or full is left alone
     * (and asserts in a debug build).
     * @param column The column.
     * @param player kMax or kMin.
     */
    void DoMove(int column, int player);

    /**
     * The top piece of `column`, which must hold one, taken back; a column off the board or empty is left alone (and
     * asserts in a debug build).
     * @param column The column.
     */
    void UndoMove(int column);

    /**
     * @param column The column, on the board.
     * @param row The row, on the board.
     * @return What is in the slot: kMax, kMin or kEmpty.
     */
    int At(int column, int row) const;

    /**
     * @param column The column, on the board.
     * @return How many pieces it holds.
     */
    int ColumnHeight(int column) const;

    /** @return Whether any column has room. */
    bool HasRoom() const;

    /**
     * Whether the piece `player` has just dropped into `column` made four in a row.
     * @param column The column of the piece just played.
     * @param player Whose piece it is.
     * @return Whether it won; false for an empty column or one off the board.
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

    /** @return The board as text, the top row first: `X` for kMax, `O` for kMin, a space for an empty slot. */
    std::string ToString() const;

  private:
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
