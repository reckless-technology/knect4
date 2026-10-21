#include "effects/drop_motion.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace knect4
{

DropMotion::DropMotion(double rest, Physics physics) : rest_(std::max(rest, 0.0)), physics_(physics)
{
    if (rest_ == 0.0)
    {
        has_settled_ = true;
    }
}

void DropMotion::Advance(double seconds)
{
    // Exactly, piece by piece: the time left is spent falling to the next hit, then rising and falling from it.
    double left = std::max(seconds, 0.0);
    while (!has_settled_ && left > 0.0)
    {
        // Where it would be after `left` under gravity, and the time to the floor if it gets there first.
        const double gravity  = physics_.gravity;
        const double distance = rest_ - position_;
        // position + v t + g t^2 / 2 = rest, the positive root.
        const double to_floor = (-velocity_ + std::sqrt(velocity_ * velocity_ + 2.0 * gravity * distance)) / gravity;
        if (to_floor > left)
        {
            position_ += velocity_ * left + 0.5 * gravity * left * left;
            velocity_ += gravity * left;
            return;
        }
        const double speed = velocity_ + gravity * to_floor;
        left -= to_floor;
        impacts_.push_back({speed, hits_});
        ++hits_;
        position_            = rest_;
        const double rebound = speed * physics_.restitution;
        if (rebound < physics_.least_rebound)
        {
            velocity_    = 0.0;
            has_settled_ = true;
            return;
        }
        velocity_ = -rebound;
    }
}

std::vector<DropMotion::Impact> DropMotion::TakeImpacts()
{
    return std::exchange(impacts_, {});
}

double DropMotion::HitSpeed(double rows, const Physics &physics)
{
    return std::sqrt(2.0 * physics.gravity * std::max(rows, 0.0));
}

} // namespace knect4
