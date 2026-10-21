#pragma once
/// @file bits.h The bit sets the engine's boards are held in: a 64-bit word for boards of up to 64 slots with their
/// sentinel row, and several words for larger ones (9 x 7 in two, 16 x 8 in three), with the few operations the search
/// needs written once for both.

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>

namespace knect4
{

/**
 * `Words` x 64 bits, the first word the lowest: what a board too large for one word is held in. It behaves as an
 * unsigned integer of that width for the operations the search uses - addition and subtraction with carries, the
 * bitwise operations and shifts.
 * @tparam Words The 64-bit words, 2 or more.
 */
template <std::size_t Words> struct WideBits
{
    static_assert(Words >= 2, "one word is a std::uint64_t");

    std::array<std::uint64_t, Words> words{}; ///< The words, the lowest first.

    /** @return None set. */
    constexpr WideBits() = default;

    /** @param bits The lowest word; the others clear. */
    constexpr WideBits(std::uint64_t bits) noexcept // NOLINT(google-explicit-constructor): as an integer
    {
        words[0] = bits;
    }

    /** @return Whether any bit is set. */
    constexpr explicit operator bool() const noexcept
    {
        for (const std::uint64_t word : words)
        {
            if (word != 0)
            {
                return true;
            }
        }
        return false;
    }

    /** @param other Bits. @return The two equal. */
    constexpr bool operator==(const WideBits &other) const noexcept = default;

    /** @param other Bits. @return Their sum, each word's carry into the next. */
    constexpr WideBits operator+(const WideBits &other) const noexcept
    {
        WideBits      sum;
        std::uint64_t carry = 0;
        for (std::size_t index = 0; index != Words; ++index)
        {
            const std::uint64_t partial = words[index] + other.words[index];
            const std::uint64_t total   = partial + carry;
            carry                       = (partial < words[index] ? 1u : 0u) + (total < partial ? 1u : 0u);
            sum.words[index]            = total;
        }
        return sum;
    }

    /** @param other Bits. @return Their difference, each word borrowing from the next. */
    constexpr WideBits operator-(const WideBits &other) const noexcept
    {
        WideBits      difference;
        std::uint64_t borrow = 0;
        for (std::size_t index = 0; index != Words; ++index)
        {
            const std::uint64_t partial = words[index] - other.words[index];
            const std::uint64_t total   = partial - borrow;
            borrow                      = (words[index] < other.words[index] ? 1u : 0u) + (partial < borrow ? 1u : 0u);
            difference.words[index]     = total;
        }
        return difference;
    }

    /** @param other Bits. @return Those in both. */
    constexpr WideBits operator&(const WideBits &other) const noexcept
    {
        WideBits result;
        for (std::size_t index = 0; index != Words; ++index)
        {
            result.words[index] = words[index] & other.words[index];
        }
        return result;
    }

    /** @param other Bits. @return Those in either. */
    constexpr WideBits operator|(const WideBits &other) const noexcept
    {
        WideBits result;
        for (std::size_t index = 0; index != Words; ++index)
        {
            result.words[index] = words[index] | other.words[index];
        }
        return result;
    }

    /** @param other Bits. @return Those in one but not both. */
    constexpr WideBits operator^(const WideBits &other) const noexcept
    {
        WideBits result;
        for (std::size_t index = 0; index != Words; ++index)
        {
            result.words[index] = words[index] ^ other.words[index];
        }
        return result;
    }

    /** @return Every bit flipped. */
    constexpr WideBits operator~() const noexcept
    {
        WideBits result;
        for (std::size_t index = 0; index != Words; ++index)
        {
            result.words[index] = ~words[index];
        }
        return result;
    }

    /** @param shift 0 up to the width. @return The bits moved up by `shift`, those past the top lost. */
    constexpr WideBits operator<<(int shift) const noexcept
    {
        WideBits          result;
        const std::size_t whole = static_cast<std::size_t>(shift) / 64;
        const int         part  = shift % 64;
        for (std::size_t index = Words; index-- > whole;)
        {
            const std::size_t from = index - whole;
            std::uint64_t     word = words[from] << part;
            if (part != 0 && from != 0)
            {
                word |= words[from - 1] >> (64 - part);
            }
            result.words[index] = word;
        }
        return result;
    }

    /** @param shift 0 up to the width. @return The bits moved down by `shift`, those past the bottom lost. */
    constexpr WideBits operator>>(int shift) const noexcept
    {
        WideBits          result;
        const std::size_t whole = static_cast<std::size_t>(shift) / 64;
        const int         part  = shift % 64;
        for (std::size_t index = 0; index + whole < Words; ++index)
        {
            const std::size_t from = index + whole;
            std::uint64_t     word = words[from] >> part;
            if (part != 0 && from + 1 < Words)
            {
                word |= words[from + 1] << (64 - part);
            }
            result.words[index] = word;
        }
        return result;
    }

    /** @param other Bits. @return This, with those of `other` kept only. */
    constexpr WideBits &operator&=(const WideBits &other) noexcept
    {
        return *this = *this & other;
    }

    /** @param other Bits. @return This, with those of `other` added. */
    constexpr WideBits &operator|=(const WideBits &other) noexcept
    {
        return *this = *this | other;
    }

    /** @param other Bits. @return This, flipped where `other` is set. */
    constexpr WideBits &operator^=(const WideBits &other) noexcept
    {
        return *this = *this ^ other;
    }
};

using Bits128 = WideBits<2>; ///< Two words: the 9 x 7 board.
using Bits192 = WideBits<3>; ///< Three words: the 16 x 8 board.

/** @param bits A word. @return How many bits are set. */
inline int CountBits(std::uint64_t bits) noexcept
{
    return std::popcount(bits);
}

/** @tparam Words The words. @param bits The bits. @return How many are set. */
template <std::size_t Words> int CountBits(const WideBits<Words> &bits) noexcept
{
    int count = 0;
    for (const std::uint64_t word : bits.words)
    {
        count += std::popcount(word);
    }
    return count;
}

/** @param bits A word. @return A hash of it for a table: the word itself, every key distinct. */
inline std::uint64_t HashBits(std::uint64_t bits) noexcept
{
    return bits;
}

/** @tparam Words The words. @param bits The bits. @return A hash of them for a table, mixed so nearby keys spread. */
template <std::size_t Words> std::uint64_t HashBits(const WideBits<Words> &bits) noexcept
{
    std::uint64_t hash = 0;
    for (const std::uint64_t word : bits.words)
    {
        hash = (hash ^ word) * 0x9e3779b97f4a7c15u;
        hash ^= hash >> 31;
    }
    hash *= 0xbf58476d1ce4e5b9u;
    return hash ^ (hash >> 29);
}

/** @param bits A word. @return Whether any bit is set. */
constexpr bool Any(std::uint64_t bits) noexcept
{
    return bits != 0;
}

/** @tparam Words The words. @param bits The bits. @return Whether any is set. */
template <std::size_t Words> constexpr bool Any(const WideBits<Words> &bits) noexcept
{
    return static_cast<bool>(bits);
}

} // namespace knect4
