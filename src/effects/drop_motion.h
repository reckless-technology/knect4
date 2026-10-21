#pragma once
/// @file drop_motion.h A puck's fall down a column: gravity from where it was let go, a hit on what it lands on, a
/// bounce or two smaller each time, and rest.

#include <vector>

namespace knect4
{

/**
 * A puck falling down a column, in rows: its height measured down from where it was let go, 0 there and growing as it
 * falls, until it rests on what lies below at `rest`. It falls under gravity, and on each hit rises again at the
 * restitution's share of the speed it hit at, until a hit is too slow to rise from. Each hit is told (TakeImpacts) with
 * its speed, for the click it makes.
 */
class DropMotion
{
  public:
    /** How the puck falls. */
    struct Physics
    {
        double gravity{90.0};      ///< The fall's acceleration, in rows a second a second.
        double restitution{0.22};  ///< The share of a hit's speed it rises again at.
        double least_rebound{1.6}; ///< A hit slower than this, in rows a second, ends the fall: the puck rests.
    };

    /** A hit on what the puck lands on. */
    struct Impact
    {
        double speed{0.0}; ///< How fast it was falling, in rows a second.
        int    number{0};  ///< 0 for the fall's first hit, 1 for the first bounce's, and so on.
    };

    /**
     * A fall from rest. A rest that is not a number, or 0 or less, is no fall: at rest at once. Physics that could not
     * end - gravity that is not more than 0, a restitution outside 0 to under 1, a least rebound that is not more than
     * 0, or any of them not a number - are replaced by the usual.
     * @param rest How far below where it was let go it comes to rest, in rows.
     * @param physics How it falls.
     */
    explicit DropMotion(double rest, Physics physics);

    /** A fall from rest with the usual physics. @param rest How far it falls, in rows. */
    explicit DropMotion(double rest) : DropMotion(rest, Physics{})
    {
    }

    /**
     * The fall moved on.
     * @param seconds The time since the last step.
     */
    void Advance(double seconds);

    /** @return How far below where it was let go the puck stands now, in rows. */
    double position() const noexcept
    {
        return position_;
    }

    /** @return Where it comes to rest. */
    double rest() const noexcept
    {
        return rest_;
    }

    /** @return Whether it has come to rest. */
    bool has_settled() const noexcept
    {
        return has_settled_;
    }

    /** @return The hits since the last call, oldest first; the list is emptied. */
    std::vector<Impact> TakeImpacts();

    /**
     * How fast a fall of `rows` hits: the speed a hit's click is measured against.
     * @param rows The height fallen.
     * @param physics How it falls.
     * @return The speed at the bottom, in rows a second.
     */
    static double HitSpeed(double rows, const Physics &physics);

  private:
    double              rest_;               ///< Where it rests.
    Physics             physics_;            ///< How it falls.
    double              position_{0.0};      ///< Where it is, down from where it was let go.
    double              velocity_{0.0};      ///< Its speed, downward positive.
    bool                has_settled_{false}; ///< At rest.
    int                 hits_{0};            ///< The hits so far.
    std::vector<Impact> impacts_;            ///< The hits not yet taken.
};

} // namespace knect4
