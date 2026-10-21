#pragma once
/// @file opening_book.h Perfect play's opening on the standard board, worked out ahead by the solver
/// (tools/make_book.cpp) and compiled in (opening_book_data.inc): the best column for each position the computer can
/// meet early in a game while it plays the book's moves, where the solver alone would take too long.

#include "game/position.h"

#include <cstdint>
#include <optional>
#include <span>

namespace knect4
{

/** A position the book knows and the column to play there. */
struct BookEntry
{
    std::uint64_t key{0};    ///< The position's key (Position::Key), the lesser of it and its mirror image's.
    int           column{0}; ///< The best column in the position the key is of.
};

/**
 * The standard board's opening book: a column for each position it holds, found by the position's key or by its
 * mirror image's - the board read right to left, its column then mirrored back. A position is found however it was
 * reached.
 */
class OpeningBook
{
  public:
    /** @return The book compiled in. */
    static const OpeningBook &Standard();

    /** @param entries The entries, sorted by key. */
    explicit OpeningBook(std::span<const BookEntry> entries) noexcept : entries_(entries)
    {
    }

    /**
     * The book's column for the position after `moves`.
     * @param moves The game's moves on the standard board.
     * @return The column to play; none where the book does not hold the position.
     */
    std::optional<int> Find(std::span<const int> moves) const;

    /** @return How many positions the book holds. */
    std::size_t size() const noexcept
    {
        return entries_.size();
    }

    /**
     * @param moves Moves on the standard board.
     * @return The key the book files the position after them under, and whether that is its mirror image's.
     */
    static std::pair<std::uint64_t, bool> CanonicalKey(std::span<const int> moves);

  private:
    std::span<const BookEntry> entries_; ///< The entries, sorted by key.
};

} // namespace knect4
