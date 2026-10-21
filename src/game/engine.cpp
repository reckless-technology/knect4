#include "game/engine.h"

#include "game/opening_book.h"

#include <algorithm>
#include <type_traits>

namespace knect4
{

/** A board size's search behind the engine's interface. */
class Engine::Sized
{
  public:
    virtual ~Sized() = default;

    /** @copydoc Engine::ChooseMove */
    virtual int ChooseMove(std::span<const int> moves, Strength strength, const std::atomic<bool> *stop,
                           double seconds) = 0;

    /** @copydoc Engine::Values */
    virtual std::vector<MoveValue> Values(std::span<const int> moves, Strength strength, double seconds,
                                          const std::atomic<bool> *stop) = 0;

    /** @copydoc Engine::Solve */
    virtual std::optional<int> Solve(std::span<const int> moves, double seconds, const std::atomic<bool> *stop) = 0;
};

namespace
{

/**
 * The search for positions of type `P`.
 * @tparam P The position type.
 */
template <typename P> class SizedSearch final : public Engine::Sized
{
  public:
    /** @param seed The choice between equal moves. */
    explicit SizedSearch(std::uint64_t seed) : search_(seed)
    {
    }

    int ChooseMove(std::span<const int> moves, Strength strength, const std::atomic<bool> *stop,
                   double seconds) override
    {
        // Perfect play on the standard board opens from the book, where the solver alone would take too long.
        if constexpr (std::is_same_v<P, Position>)
        {
            if (strength == Strength::kPerfect)
            {
                if (const std::optional<int> column = OpeningBook::Standard().Find(moves))
                {
                    return *column;
                }
            }
        }
        return search_.ChooseMove(P::FromMoves(moves), strength, stop, seconds);
    }

    std::vector<MoveValue> Values(std::span<const int> moves, Strength strength, double seconds,
                                  const std::atomic<bool> *stop) override
    {
        return search_.Values(P::FromMoves(moves), strength, seconds, stop);
    }

    std::optional<int> Solve(std::span<const int> moves, double seconds, const std::atomic<bool> *stop) override
    {
        return search_.Solve(P::FromMoves(moves), seconds, stop);
    }

  private:
    Search<P> search_; ///< The search.
};

} // namespace

bool IsOffered(BoardSize size) noexcept
{
    return std::find(kBoardSizes.begin(), kBoardSizes.end(), size) != kBoardSizes.end();
}

Engine::Engine(BoardSize size, std::uint64_t seed) : size_(IsOffered(size) ? size : kStandardBoard)
{
    if (size_ == BoardSize{6, 5})
    {
        search_ = std::make_unique<SizedSearch<BasicPosition<6, 5, std::uint64_t>>>(seed);
    }
    else if (size_ == BoardSize{8, 7})
    {
        search_ = std::make_unique<SizedSearch<BasicPosition<8, 7, std::uint64_t>>>(seed);
    }
    else if (size_ == BoardSize{9, 7})
    {
        search_ = std::make_unique<SizedSearch<BasicPosition<9, 7, Bits128>>>(seed);
    }
    else if (size_ == BoardSize{16, 8})
    {
        search_ = std::make_unique<SizedSearch<BasicPosition<16, 8, Bits192>>>(seed);
    }
    else
    {
        search_ = std::make_unique<SizedSearch<Position>>(seed);
    }
}

Engine::~Engine() = default;

int Engine::ChooseMove(std::span<const int> moves, Strength strength, const std::atomic<bool> *stop, double seconds)
{
    return search_->ChooseMove(moves, strength, stop, seconds);
}

std::vector<MoveValue> Engine::Values(std::span<const int> moves, Strength strength, double seconds,
                                      const std::atomic<bool> *stop)
{
    return search_->Values(moves, strength, seconds, stop);
}

std::optional<int> Engine::Solve(std::span<const int> moves, double seconds, const std::atomic<bool> *stop)
{
    return search_->Solve(moves, seconds, stop);
}

} // namespace knect4
