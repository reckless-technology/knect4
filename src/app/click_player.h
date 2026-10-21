#pragma once
/// @file click_player.h The landings' clicks played on the system's own output, through the toolkit's audio device
/// layer (AudioDevice.h): open only while pucks are falling, silent where there is no output to open.

#include "controller.h"

#include "effects/click_sound.h"

#include "AudioDevice.h"

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace knect4::app
{

/**
 * The clicks on the system's default output: the default API's default output device opened at its own rate, stereo,
 * a click rendered for each landing kind and seed at that rate, and the stream running a ClickMixer.
 *
 * The stream is open only while the game makes sound: it opens when a puck starts to fall (Prepare - the fall gives it
 * the time) and closes once nothing has sounded for kIdleSeconds, so a window left open neither runs an output nor
 * keeps the machine from sleeping. A stream that fails (a device unplugged) is closed at the next Frame, and an open
 * that fails is tried again at the next fall, no sooner than kRetrySeconds after. Where nothing opens, the clicks go
 * nowhere and the game plays on in silence.
 */
class ClickPlayer final : public ClickSink, private reckless::audio::AudioCallback
{
  public:
    static constexpr int    kVariations   = 4;    ///< Clicks rendered for each landing kind, played in turn.
    static constexpr double kIdleSeconds  = 30.0; ///< The quiet after which the stream is closed.
    static constexpr double kRetrySeconds = 2.0;  ///< The least wait before an open that failed is tried again.

    /** Makes the audio API to open: the machine's default, or a suite's offline one. */
    using SystemMaker = std::function<std::unique_ptr<reckless::audio::AudioSystem>()>;

    /**
     * Nothing opened yet.
     * @param make_system Makes the API to open; empty for the machine's default (reckless::audio::DefaultAudioApi).
     */
    explicit ClickPlayer(SystemMaker make_system = {});

    /** The stream stopped and closed. */
    ~ClickPlayer() override;

    ClickPlayer(const ClickPlayer &)            = delete;
    ClickPlayer &operator=(const ClickPlayer &) = delete;

    /** The output opened if it is not: the API's default output device, else its first with an output. */
    void Prepare() override;

    /**
     * A click played, if the output is open.
     * @param landing What the puck landed on.
     * @param gain How loud, 0 to 1.
     */
    void Click(LandingOn landing, float gain) override;

    /**
     * Once a frame: a stream that failed closed, a quiet one closed after kIdleSeconds, the wait before a retry run
     * down.
     * @param seconds The time since the last frame.
     */
    void Frame(double seconds);

    /** @return Whether a stream runs. */
    bool IsRunning() const noexcept
    {
        return is_running_;
    }

    /** @return Why the last open failed or the stream stopped, for the log; empty while it runs. */
    const std::string &error() const noexcept
    {
        return error_;
    }

    /** @return The clicks the mixer plays. */
    const ClickMixer &mixer() const noexcept
    {
        return mixer_;
    }

  private:
    void Process(const float *const *inputs, int input_channels, float *const *outputs, int output_channels, int frames,
                 std::int64_t now, bool xrun) noexcept override;
    void StreamFailed() noexcept override;

    /** @return Whether a stream was opened and started; else error() says why. */
    bool Open();

    /** The stream closed. */
    void Close();

    SystemMaker                                   make_system_;       ///< Makes the API to open.
    std::unique_ptr<reckless::audio::AudioSystem> system_;            ///< The API, while a stream is open.
    ClickMixer                                    mixer_;             ///< The clicks.
    std::atomic<bool>                             has_failed_{false}; ///< The stream stopped of its own accord.
    bool                                          is_running_{false}; ///< A stream runs.
    double                                        quiet_for_{0.0};    ///< Seconds since a click last sounded.
    double                                        retry_in_{0.0}; ///< Seconds until a failed open may be tried again.
    std::string                                   error_;         ///< Why it is not running.
    unsigned                                      next_{0};       ///< The next variation to play.
};

} // namespace knect4::app
