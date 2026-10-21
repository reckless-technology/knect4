#pragma once
/// @file position.h A position of four in a row held as two bitboards for the engine's search, for a board of any of
/// the sizes the game offers: whose stones are whose, as a mask of every stone and the side to move's own, each column
/// `Height + 1` bits - its slots and one above them that is always empty.

#include "game/bits.h"

#include <cstdint>
#include <span>
#include <string>

namespace knect4
{

/**
 * A position on a `Width` x `Height` board for the search, its bits in `Bits` - a 64-bit word where the board and its
 * sentinel row fit, else Bits128. A column's slots are bits c(Height + 1) to c(Height + 1) + Height - 1, the bottom
 * first; the bit above them is always empty, so a carry out of a column stops there. `mask` holds every stone and
 * `current` the stones of the side to move, so the other side's are `mask ^ current`; playing a stone flips `current`
 * to the other side's stones and adds the new one to `mask`, so `current + mask` is a key no two positions share.
 * @tparam Width The board's columns.
 * @tparam Height Its rows.
 * @tparam Bits The bit set: std::uint64_t or Bits128, at least (Height + 1) x Width bits.
 */
template <int Width, int Height, typename Bits> class BasicPosition
{
  public:
    static constexpr int kWidth  = Width;          ///< The board's columns.
    static constexpr int kHeight = Height;         ///< Its rows.
    static constexpr int kSlots  = Width * Height; ///< Its slots.
    using BitsType               = Bits;           ///< The bit set.

    static_assert(Width >= 4 && Height >= 4, "four in a row needs a board of at least four by four");
    static_assert((Height + 1) * Width <= static_cast<int>(sizeof(Bits) * 8), "the board and its sentinels must fit");

    /** The empty board, the first player to move. */
    BasicPosition() = default;

    /**
     * The position after these moves from the empty board, the first player's first.
     * @param columns The columns played, in order; a move into a full column, off the board, or after a four ends the
     *        list there.
     * @param played Set to how many of the moves were played; may be null.
     * @return The position.
     */
    static BasicPosition FromMoves(std::span<const int> columns, int *played = nullptr)
    {
        BasicPosition position;
        int           count = 0;
        for (const int column : columns)
        {
            if (!position.CanPlay(column) || HasFour(position.current_ ^ position.mask_))
            {
                break;
            }
            position.Play(column);
            ++count;
        }
        if (played != nullptr)
        {
            *played = count;
        }
        return position;
    }

    /**
     * @param column A column.
     * @return Whether a stone can be played there: on the board and with room.
     */
    bool CanPlay(int column) const noexcept
    {
        return column >= 0 && column < Width && !Any(mask_ & TopMask(column));
    }

    /**
     * A stone of the side to move played in `column`, which must have room; then it is the other side's move.
     * @param column The column.
     */
    void Play(int column) noexcept
    {
        PlayMove((mask_ + BottomMask(column)) & ColumnMask(column));
    }

    /**
     * A stone played at `move`, a bit of the slot it lands in.
     * @param move The slot's bit, one of Possible()'s.
     */
    void PlayMove(Bits move) noexcept
    {
        current_ ^= mask_;
        mask_ |= move;
        ++moves_;
    }

    /**
     * @param column A column with room.
     * @return Whether a stone of the side to move there makes four.
     */
    bool IsWinningMove(int column) const noexcept
    {
        return Any(WinningPositions() & Possible() & ColumnMask(column));
    }

    /** @return Whether the side to move can make four with its next stone. */
    bool CanWinNext() const noexcept
    {
        return Any(WinningPositions() & Possible());
    }

    /** @return The stones played so far. */
    int moves() const noexcept
    {
        return moves_;
    }

    /** @return A key no other position shares, for the search's table. */
    Bits Key() const noexcept
    {
        return current_ + mask_;
    }

    /** @return The slots a stone can be played into now: the lowest empty slot of each column with room. */
    Bits Possible() const noexcept
    {
        return (mask_ + kBottomMask) & kBoardMask;
    }

    /**
     * The moves that do not hand the other side a four at once: none when it already has two threats the side to move
     * cannot both stop, the one block when it has one, else every move but those that let it win on top of them. Valid
     * only where the side to move cannot win at once (CanWinNext() is false).
     * @return The slots, as bits.
     */
    Bits PossibleNonLosingMoves() const noexcept
    {
        Bits       possible      = Possible();
        const Bits opponent_wins = OpponentWinningPositions();
        const Bits forced_moves  = possible & opponent_wins;
        if (Any(forced_moves))
        {
            if (Any(forced_moves & (forced_moves - Bits{1})))
            {
                return Bits{}; // two threats to stop at once: every move loses
            }
            possible = forced_moves;
        }
        // Never play just under a slot that would give the other side four.
        return possible & ~(opponent_wins >> 1);
    }

    /**
     * How good a move looks before it is searched: the slots it would leave the side to move one stone from four.
     * @param move A slot's bit.
     * @return The count.
     */
    int MoveScore(Bits move) const noexcept
    {
        return CountBits(ComputeWinningPositions(current_ | move, mask_));
    }

    /** @return The empty slots that would give the side to move four. */
    Bits WinningPositions() const noexcept
    {
        return ComputeWinningPositions(current_, mask_);
    }

    /** @return The empty slots that would give the other side four. */
    Bits OpponentWinningPositions() const noexcept
    {
        return ComputeWinningPositions(current_ ^ mask_, mask_);
    }

    /** @return The side to move's stones. */
    Bits current() const noexcept
    {
        return current_;
    }

    /** @return Every stone. */
    Bits mask() const noexcept
    {
        return mask_;
    }

    /** @return The board as text, the top row first: X for the first player's stones, O for the second's. */
    std::string ToString() const
    {
        const Bits  first = moves_ % 2 == 0 ? current_ : current_ ^ mask_;
        std::string text;
        for (int row = Height - 1; row >= 0; --row)
        {
            text += '|';
            for (int column = 0; column != Width; ++column)
            {
                const Bits slot = BottomMask(column) << row;
                text += !Any(mask_ & slot) ? ' ' : (Any(first & slot) ? 'X' : 'O');
                text += '|';
            }
            text += '\n';
        }
        return text;
    }

    /**
     * @param column A column.
     * @return Its column's lowest slot.
     */
    static constexpr Bits BottomMask(int column) noexcept
    {
        return Bits{1} << (column * (Height + 1));
    }

    /**
     * @param column A column.
     * @return Its column's top slot.
     */
    static constexpr Bits TopMask(int column) noexcept
    {
        return Bits{1} << (Height - 1 + column * (Height + 1));
    }

    /**
     * @param column A column.
     * @return Its column's slots.
     */
    static constexpr Bits ColumnMask(int column) noexcept
    {
        const Bits slots{(std::uint64_t{1} << Height) - 1};
        return slots << (column * (Height + 1));
    }

    /**
     * Every row whose index, counted from 0 at the bottom, has `parity`, across the board.
     * @param parity 0 for the 1st, 3rd, 5th... rows from the bottom; 1 for the 2nd, 4th...
     * @return The slots.
     */
    static constexpr Bits RowsOfParity(int parity) noexcept
    {
        std::uint64_t column = 0;
        for (int row = parity; row < Height; row += 2)
        {
            column |= std::uint64_t{1} << row;
        }
        Bits rows{};
        for (int index = 0; index != Width; ++index)
        {
            rows |= Bits{column} << (index * (Height + 1));
        }
        return rows;
    }

    /**
     * @param stones Stones, as bits.
     * @return Whether four of them stand in a line.
     */
    static bool HasFour(Bits stones) noexcept
    {
        // Across, up and the two diagonals: each a shift between neighbouring slots in its direction.
        for (const int shift : {Height + 1, 1, Height, Height + 2})
        {
            const Bits pairs = stones & (stones >> shift);
            if (Any(pairs & (pairs >> (2 * shift))))
            {
                return true;
            }
        }
        return false;
    }

    /**
     * The empty slots that would make four for the stones `position`.
     * @param position One side's stones.
     * @param mask Every stone.
     * @return The slots.
     */
    static Bits ComputeWinningPositions(Bits position, Bits mask) noexcept
    {
        // Up: three stacked with room on top.
        Bits result = (position << 1) & (position << 2) & (position << 3);
        // Across and the two diagonals: every slot that completes three in its line from either side or the middle.
        for (const int shift : {Height + 1, Height, Height + 2})
        {
            Bits pairs = (position << shift) & (position << (2 * shift));
            result |= pairs & (position << (3 * shift));
            result |= pairs & (position >> shift);
            pairs = (position >> shift) & (position >> (2 * shift));
            result |= pairs & (position << shift);
            result |= pairs & (position >> (3 * shift));
        }
        return result & (kBoardMask ^ mask);
    }

    /** The lowest slot of every column. */
    static constexpr Bits kBottomMask = [] {
        Bits bottom{};
        for (int column = 0; column != Width; ++column)
        {
            bottom |= BottomMask(column);
        }
        return bottom;
    }();

    /** Every slot of the board. */
    static constexpr Bits kBoardMask = [] {
        Bits board{};
        for (int column = 0; column != Width; ++column)
        {
            board |= ColumnMask(column);
        }
        return board;
    }();

  private:
    Bits current_{}; ///< The side to move's stones.
    Bits mask_{};    ///< Every stone.
    int  moves_{0};  ///< Stones played.
};

/** The standard board, 7 x 6. */
using Position = BasicPosition<7, 6, std::uint64_t>;

} // namespace knect4
