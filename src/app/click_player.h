#pragma once
/// @file click_player.h The landings' clicks played on the system's own output, through the toolkit's audio device
/// layer (AudioDevice.h); silent where there is no output to open.

#include "controller.h"

#include "effects/click_sound.h"

#include "AudioDevice.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>

namespace knect4::app
{

/**
 * The clicks on the system's default output: the default API's default output device opened at its own rate, stereo,
 * a click rendered for each landing kind and seed at that rate, and the stream running a ClickMixer. A stream that
 * fails (a device unplugged) is closed and opened again on the next Frame a couple of seconds on. Where nothing opens,
 * the clicks go nowhere and the game plays on in silence.
 */
class ClickPlayer final : public ClickSink, private reckless::audio::AudioCallback
{
  public:
    static constexpr int kVariations = 4; ///< Clicks rendered for each landing kind, played in turn.

    ClickPlayer();

    /** The stream stopped and closed. */
    ~ClickPlayer() override;

    ClickPlayer(const ClickPlayer &)            = delete;
    ClickPlayer &operator=(const ClickPlayer &) = delete;

    /** The output opened: the default API's default output device. @return Whether a stream runs. */
    bool Open();

    /**
     * Once a frame: a stream that failed is closed, and opened again once `kRetrySeconds` have passed.
     * @param seconds The time since the last frame.
     */
    void Frame(double seconds);

    /** @return Whether a stream runs. */
    bool IsRunning() const noexcept
    {
        return is_running_;
    }

    /** @return Why the output could not be opened, for the log; empty when it runs. */
    const std::string &error() const noexcept
    {
        return error_;
    }

    void Click(LandingOn landing, float gain) override;

  private:
    static constexpr double kRetrySeconds = 2.0; ///< The wait before a failed stream is opened again.

    void Process(const float *const *inputs, int input_channels, float *const *outputs, int output_channels, int frames,
                 std::int64_t now, bool xrun) noexcept override;
    void StreamFailed() noexcept override;

    /** The stream closed. */
    void Close();

    std::unique_ptr<reckless::audio::AudioSystem> system_;            ///< The API, while one is open.
    ClickMixer                                    mixer_;             ///< The clicks.
    std::atomic<bool>                             has_failed_{false}; ///< The stream stopped of its own accord.
    bool                                          is_running_{false}; ///< A stream runs.
    double                                        retry_in_{0.0};     ///< Seconds until it is opened again.
    std::string                                   error_;             ///< Why it is not running.
    unsigned                                      next_{0};           ///< The next variation to play.
};

} // namespace knect4::app
