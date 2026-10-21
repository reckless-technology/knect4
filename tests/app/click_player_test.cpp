#include "app/click_player.h"

#include "OfflineDevice.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

namespace knect4::app
{
namespace
{

/**
 * A click player on the toolkit's offline audio API, which runs its stream only when the suite says: each open makes a
 * new offline system, kept here while the player holds it, configured by the suite first.
 */
class ClickPlayerTest : public ::testing::Test
{
  protected:
    ClickPlayerTest()
        : player_([this]() -> std::unique_ptr<reckless::audio::AudioSystem> {
              ++makes_;
              if (is_api_missing_)
              {
                  return nullptr;
              }
              auto system = devices_.empty() ? std::make_unique<reckless::audio::OfflineAudioSystem>(0, 2)
                                             : std::make_unique<reckless::audio::OfflineAudioSystem>(devices_);
              system->RefuseOpen(does_refuse_open_);
              system->RefuseStart(does_refuse_start_);
              system_ = system.get();
              return system;
          })
    {
    }

    /**
     * One block of the stream run, as the device would.
     * @return The block's left channel.
     */
    std::vector<float> RunBlock()
    {
        std::array<std::vector<float>, 2> channels{std::vector<float>(256), std::vector<float>(256)};
        std::array<float *, 2>            outputs{channels[0].data(), channels[1].data()};
        system_->Run(256, nullptr, outputs.data(), 0);
        return channels[0];
    }

    /**
     * @param block A block's samples.
     * @return The loudest of them.
     */
    static float Peak(const std::vector<float> &block)
    {
        float peak = 0.0f;
        for (const float sample : block)
        {
            peak = std::max(peak, std::abs(sample));
        }
        return peak;
    }

    int                                      makes_{0};                 ///< The systems made.
    bool                                     is_api_missing_{false};    ///< The maker makes none.
    bool                                     does_refuse_open_{false};  ///< The next system refuses to open.
    bool                                     does_refuse_start_{false}; ///< It refuses to start.
    std::vector<reckless::audio::DeviceInfo> devices_;                  ///< Its devices; empty for one stereo output.
    reckless::audio::OfflineAudioSystem     *system_{nullptr};          ///< The system the player holds.
    ClickPlayer                              player_;                   ///< The player.
};

TEST_F(ClickPlayerTest, OpensOnlyWhenAPuckFallsAndThenPlaysItsClicks)
{
    EXPECT_FALSE(player_.IsRunning());
    player_.Click(LandingOn::kBase, 1.0f); // nothing open: nothing to play, nothing opened
    EXPECT_EQ(makes_, 0);

    player_.Prepare();
    ASSERT_TRUE(player_.IsRunning());
    EXPECT_TRUE(player_.error().empty());
    EXPECT_EQ(system_->request().output_count, 2);
    EXPECT_EQ(system_->request().output_device, "Offline");
    EXPECT_EQ(Peak(RunBlock()), 0.0f); // open, and silent until a click

    player_.Click(LandingOn::kBase, 1.0f);
    EXPECT_GT(Peak(RunBlock()), 0.5f);
    for (int block = 0; block != 40; ++block) // the click's tenth of a second and more, played out
    {
        RunBlock();
    }
    ASSERT_EQ(player_.mixer().SoundingCount(), 0);
    player_.Click(LandingOn::kPuck, 0.25f);
    const float quiet = Peak(RunBlock());
    EXPECT_GT(quiet, 0.0f);
    EXPECT_LT(quiet, 0.5f);
    EXPECT_GT(player_.mixer().SoundingCount(), 0);

    // A second fall finds it open: no second stream.
    player_.Prepare();
    EXPECT_EQ(makes_, 1);
}

TEST_F(ClickPlayerTest, ClosesAfterAQuietSpellAndOpensAgainForTheNextFall)
{
    player_.Prepare();
    ASSERT_TRUE(player_.IsRunning());
    player_.Click(LandingOn::kBase, 1.0f);
    for (int block = 0; block != 200; ++block) // the click played out
    {
        RunBlock();
    }
    ASSERT_EQ(player_.mixer().SoundingCount(), 0);
    player_.Frame(ClickPlayer::kIdleSeconds / 2.0);
    EXPECT_TRUE(player_.IsRunning());
    player_.Frame(ClickPlayer::kIdleSeconds / 2.0 + 0.1);
    EXPECT_FALSE(player_.IsRunning());
    EXPECT_TRUE(player_.error().empty()); // closed, not failed

    player_.Prepare();
    EXPECT_TRUE(player_.IsRunning());
    EXPECT_EQ(makes_, 2);
}

TEST_F(ClickPlayerTest, StaysOpenWhileItSounds)
{
    player_.Prepare();
    player_.Click(LandingOn::kBase, 1.0f);
    RunBlock();
    player_.Frame(ClickPlayer::kIdleSeconds + 1.0); // a click still sounding: not quiet
    EXPECT_TRUE(player_.IsRunning());
}

TEST_F(ClickPlayerTest, ClosesAStreamThatFailsAndOpensAnotherAtTheNextFall)
{
    player_.Prepare();
    ASSERT_TRUE(player_.IsRunning());
    system_->Fail();                  // the device unplugged
    EXPECT_TRUE(player_.IsRunning()); // until the window's thread hears of it
    player_.Frame(1.0 / 60.0);
    EXPECT_FALSE(player_.IsRunning());
    EXPECT_EQ(player_.error(), "the output stopped");

    player_.Prepare();
    EXPECT_TRUE(player_.IsRunning());
    EXPECT_TRUE(player_.error().empty());
}

TEST_F(ClickPlayerTest, TriesAFailedOpenAgainNoSoonerThanItsWait)
{
    does_refuse_open_ = true;
    player_.Prepare();
    EXPECT_FALSE(player_.IsRunning());
    EXPECT_NE(player_.error().find("refused"), std::string::npos);
    player_.Prepare(); // straight after: not tried
    EXPECT_EQ(makes_, 1);
    player_.Frame(ClickPlayer::kRetrySeconds / 2.0);
    player_.Prepare();
    EXPECT_EQ(makes_, 1);

    does_refuse_open_ = false;
    player_.Frame(ClickPlayer::kRetrySeconds);
    player_.Prepare();
    EXPECT_EQ(makes_, 2);
    EXPECT_TRUE(player_.IsRunning());
}

TEST_F(ClickPlayerTest, SaysWhyNothingOpened)
{
    does_refuse_start_ = true;
    player_.Prepare();
    EXPECT_FALSE(player_.IsRunning());
    EXPECT_NE(player_.error().find("would not start"), std::string::npos);

    // A device with no output.
    reckless::audio::DeviceInfo input_only;
    input_only.name           = "Microphone";
    input_only.input_channels = 2;
    devices_                  = {input_only};
    does_refuse_start_        = false;
    player_.Frame(ClickPlayer::kRetrySeconds);
    player_.Prepare();
    EXPECT_FALSE(player_.IsRunning());
    EXPECT_NE(player_.error().find("no output device"), std::string::npos);

    // No API at all.
    is_api_missing_ = true;
    player_.Frame(ClickPlayer::kRetrySeconds);
    player_.Prepare();
    EXPECT_EQ(player_.error(), "no audio API");
}

TEST_F(ClickPlayerTest, TakesAnOutputThatIsNotTheDefaultWhereNoneIs)
{
    reckless::audio::DeviceInfo speakers;
    speakers.name            = "Speakers";
    speakers.output_channels = 6;
    devices_                 = {speakers};
    player_.Prepare();
    ASSERT_TRUE(player_.IsRunning());
    EXPECT_EQ(system_->request().output_device, "Speakers");
    EXPECT_EQ(system_->request().output_count, 2); // stereo is enough for a click
}

TEST(ClickPlayer, OpensTheMachinesOwnOutputOrSaysWhyNot)
{
    // The machine's default API: a desk has an output, a CI runner perhaps none. Either way it answers.
    ClickPlayer player;
    player.Prepare();
    EXPECT_TRUE(player.IsRunning() || !player.error().empty());
}

} // namespace
} // namespace knect4::app
