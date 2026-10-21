#include "effects/click_sound.h"
#include "effects/drop_motion.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace connect4
{
namespace
{

TEST(DropMotion, FallsUnderGravityToItsRest)
{
    DropMotion motion(5.0);
    EXPECT_EQ(motion.position(), 0.0);
    motion.Advance(0.1);
    // Half g t squared: 90 / 2 x 0.01.
    EXPECT_NEAR(motion.position(), 0.45, 1e-9);
    EXPECT_FALSE(motion.has_settled());
    EXPECT_TRUE(motion.TakeImpacts().empty());
}

TEST(DropMotion, HitsBouncesAndSettles)
{
    DropMotion          motion(6.0);
    std::vector<double> speeds;
    double              highest_bounce = 6.0; // the least position after the first hit
    bool                has_hit        = false;
    for (int step = 0; step != 600 && !motion.has_settled(); ++step)
    {
        motion.Advance(1.0 / 120.0);
        EXPECT_LE(motion.position(), 6.0 + 1e-9);
        for (const DropMotion::Impact &impact : motion.TakeImpacts())
        {
            EXPECT_EQ(impact.number, static_cast<int>(speeds.size()));
            speeds.push_back(impact.speed);
            has_hit = true;
        }
        if (has_hit)
        {
            highest_bounce = std::min(highest_bounce, motion.position());
        }
    }
    ASSERT_TRUE(motion.has_settled());
    EXPECT_EQ(motion.position(), 6.0);
    // The first hit at the speed of the whole fall, then each smaller; a bounce, but a small one.
    ASSERT_GE(speeds.size(), 2u);
    EXPECT_NEAR(speeds[0], DropMotion::HitSpeed(6.0, {}), 1e-9);
    for (std::size_t index = 1; index < speeds.size(); ++index)
    {
        EXPECT_LT(speeds[index], speeds[index - 1]);
    }
    EXPECT_LT(highest_bounce, 6.0);
    EXPECT_GT(highest_bounce, 5.5);
}

TEST(DropMotion, SpendsALongStepAcrossItsBounces)
{
    // One step longer than the whole fall: every hit told, and at rest.
    DropMotion motion(4.0);
    motion.Advance(5.0);
    EXPECT_TRUE(motion.has_settled());
    EXPECT_GE(motion.TakeImpacts().size(), 2u);
    EXPECT_TRUE(motion.TakeImpacts().empty());
    // No distance to fall: at rest from the start.
    EXPECT_TRUE(DropMotion(0.0).has_settled());
}

TEST(ClickSound, IsShortLoudAtOnceAndEndsInSilence)
{
    for (const int rate : {44100, 48000, 96000})
    {
        for (const LandingOn landing : {LandingOn::kBase, LandingOn::kPuck})
        {
            const std::vector<float> click = RenderClick(rate, landing, 3);
            ASSERT_EQ(click.size(), static_cast<std::size_t>(0.15 * rate));
            float       peak    = 0.0f;
            std::size_t at_peak = 0;
            for (std::size_t index = 0; index < click.size(); ++index)
            {
                if (std::abs(click[index]) > peak)
                {
                    peak    = std::abs(click[index]);
                    at_peak = index;
                }
            }
            EXPECT_NEAR(peak, 0.8f, 1e-5f);
            // Loudest in its first two milliseconds: a click, not a swell.
            EXPECT_LT(at_peak, static_cast<std::size_t>(0.002 * rate));
            // Its last fiftieth of a second all but silent, its last sample silent.
            float tail = 0.0f;
            for (std::size_t index = click.size() - click.size() / 7; index < click.size(); ++index)
            {
                tail = std::max(tail, std::abs(click[index]));
            }
            EXPECT_LT(tail, 0.02f);
            EXPECT_EQ(click.back(), 0.0f);
        }
    }
}

TEST(ClickSound, VariesBySeedAndRepeatsByIt)
{
    EXPECT_EQ(RenderClick(48000, LandingOn::kPuck, 1), RenderClick(48000, LandingOn::kPuck, 1));
    EXPECT_NE(RenderClick(48000, LandingOn::kPuck, 1), RenderClick(48000, LandingOn::kPuck, 2));
    EXPECT_NE(RenderClick(48000, LandingOn::kPuck, 1), RenderClick(48000, LandingOn::kBase, 1));
}

/** One block rendered into two channels. */
struct Block
{
    std::array<std::vector<float>, 2> channels{std::vector<float>(64), std::vector<float>(64)};
    std::array<float *, 2>            pointers{channels[0].data(), channels[1].data()};

    /** @param mixer The mixer to render. */
    void Render(ClickMixer &mixer)
    {
        mixer.Render(pointers.data(), 2, 64);
    }
};

TEST(ClickMixer, IsSilentUntilAskedThenPlaysTheSoundAtItsGain)
{
    ClickMixer mixer;
    mixer.SetSounds({std::vector<float>(100, 0.5f)});
    Block block;
    block.Render(mixer);
    EXPECT_TRUE(std::all_of(block.channels[0].begin(), block.channels[0].end(), [](float s) { return s == 0.0f; }));

    ASSERT_TRUE(mixer.Trigger(0, 0.5f));
    block.Render(mixer);
    for (const auto &channel : block.channels)
    {
        EXPECT_TRUE(std::all_of(channel.begin(), channel.end(), [](float s) { return s == 0.25f; }));
    }
    EXPECT_EQ(mixer.SoundingCount(), 1);
    // The last 36 samples, then silence.
    block.Render(mixer);
    EXPECT_EQ(block.channels[0][35], 0.25f);
    EXPECT_EQ(block.channels[0][36], 0.0f);
    EXPECT_EQ(mixer.SoundingCount(), 0);
}

TEST(ClickMixer, SumsClicksAndIgnoresWhatItCannotPlay)
{
    ClickMixer mixer;
    mixer.SetSounds({std::vector<float>(1000, 0.1f), std::vector<float>(1000, 0.2f)});
    EXPECT_FALSE(mixer.Trigger(2, 1.0f));
    EXPECT_FALSE(mixer.Trigger(-1, 1.0f));
    ASSERT_TRUE(mixer.Trigger(0, 1.0f));
    ASSERT_TRUE(mixer.Trigger(1, 1.0f));
    Block block;
    block.Render(mixer);
    EXPECT_NEAR(block.channels[1][10], 0.3f, 1e-6f);
    EXPECT_EQ(mixer.SoundingCount(), 2);
}

TEST(ClickMixer, KeepsEightVoicesAndDropsWhatOverflowsItsQueue)
{
    ClickMixer mixer;
    mixer.SetSounds({std::vector<float>(10000, 0.01f)});
    for (int count = 0; count != ClickMixer::kQueueSize; ++count)
    {
        EXPECT_TRUE(mixer.Trigger(0, 1.0f));
    }
    EXPECT_FALSE(mixer.Trigger(0, 1.0f)); // the queue is full until a block takes them
    Block block;
    block.Render(mixer);
    EXPECT_EQ(mixer.SoundingCount(), ClickMixer::kVoices);
    EXPECT_NEAR(block.channels[0][0], 0.01f * ClickMixer::kVoices, 1e-6f);
    EXPECT_TRUE(mixer.Trigger(0, 1.0f));
}

} // namespace
} // namespace connect4
