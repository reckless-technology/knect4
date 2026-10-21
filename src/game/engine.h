#pragma once
/// @file engine.h The computer's play on any of the board sizes the game offers, behind one interface: the game's moves
/// in, a column or the moves' values out.

#include "game/search.h"

#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <optional>
#include <random>
#include <span>
#include <vector>

namespace knect4
{

/** A board's size. */
struct BoardSize
{
    int columns{7}; ///< Across.
    int rows{6};    ///< Down.

    /** @param other Another size. @return The two the same. */
    bool operator==(const BoardSize &other) const = default;
};

/** The sizes the game offers: small, the standard board, two larger ones and the supersize board. */
inline constexpr std::array<BoardSize, 5> kBoardSizes{{{6, 5}, {7, 6}, {8, 7}, {9, 7}, {16, 8}}};

/** The standard board. */
inline constexpr BoardSize kStandardBoard{7, 6};

/**
 * @param size A size.
 * @return Whether the game offers it.
 */
bool IsOffered(BoardSize size) noexcept;

/**
 * The computer's play on a board of one of the offered sizes (kBoardSizes): a Search on that size's positions, made
 * when the engine is, its table with it. Positions are given as the game's moves from the empty board.
 */
class Engine
{
  public:
    /**
     * @param size The board's size; one not offered is taken as the standard board.
     * @param seed The choice between equal moves.
     */
    explicit Engine(BoardSize size = kStandardBoard, std::uint64_t seed = std::random_device{}());

    ~Engine();
    Engine(const Engine &)            = delete;
    Engine &operator=(const Engine &) = delete;

    /** @return The board's size. */
    BoardSize size() const noexcept
    {
        return size_;
    }

    /**
     * The column the side to move plays at `strength` (Search::ChooseMove).
     * @param moves The game's moves from the empty board; the game is not over.
     * @param strength How well it plays.
     * @param stop Read as it searches; may be null.
     * @param seconds The time it may take; 0 for the level's own.
     * @return The column.
     */
    int ChooseMove(std::span<const int> moves, Strength strength, const std::atomic<bool> *stop = nullptr,
                   double seconds = 0.0);

    /**
     * Every possible move's value for the side to move (Search::Values).
     * @param moves The game's moves; the game is not over.
     * @param strength How deep to look.
     * @param seconds The time allowed.
     * @param stop Read as it searches; may be null.
     * @return Each column with room and its value.
     */
    std::vector<MoveValue> Values(std::span<const int> moves, Strength strength, double seconds,
                                  const std::atomic<bool> *stop = nullptr);

    /**
     * The game's exact score with best play (Search::Solve).
     * @param moves The game's moves; the game is not over.
     * @param seconds The time allowed.
     * @param stop Read as it searches; may be null.
     * @return The score in moves; none when not found in time.
     */
    std::optional<int> Solve(std::span<const int> moves, double seconds, const std::atomic<bool> *stop = nullptr);

    /** The searches for each size, behind one interface. */
    class Sized;

  private:
    BoardSize              size_;   ///< The board's size.
    std::unique_ptr<Sized> search_; ///< The search for it.
};

} // namespace knect4
