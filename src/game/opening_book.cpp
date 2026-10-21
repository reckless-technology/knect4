#include "game/opening_book.h"

#include <algorithm>
#include <array>
#include <vector>

namespace knect4
{

namespace
{

/** The book, as the generator wrote it: sorted by key. */
constexpr BookEntry kStandardEntries[] = {
#include "game/opening_book_data.inc"
};

} // namespace

const OpeningBook &OpeningBook::Standard()
{
    static const OpeningBook book{std::span<const BookEntry>(kStandardEntries)};
    return book;
}

std::pair<std::uint64_t, bool> OpeningBook::CanonicalKey(std::span<const int> moves)
{
    std::vector<int> mirrored(moves.begin(), moves.end());
    for (int &column : mirrored)
    {
        column = Position::kWidth - 1 - column;
    }
    const std::uint64_t key        = Position::FromMoves(moves).Key();
    const std::uint64_t mirror_key = Position::FromMoves(mirrored).Key();
    return mirror_key < key ? std::pair{mirror_key, true} : std::pair{key, false};
}

std::optional<int> OpeningBook::Find(std::span<const int> moves) const
{
    const auto [key, is_mirrored] = CanonicalKey(moves);
    const auto found =
        std::lower_bound(entries_.begin(), entries_.end(), key,
                         [](const BookEntry &entry, std::uint64_t wanted) { return entry.key < wanted; });
    if (found == entries_.end() || found->key != key)
    {
        return std::nullopt;
    }
    return is_mirrored ? Position::kWidth - 1 - found->column : found->column;
}

} // namespace knect4
