#pragma once
/// @file click_sound.h The click a puck makes as it lands, made from nothing at the output's rate, and the mixer that
/// plays clicks on an audio thread while the window's thread asks for them.

#include <array>
#include <atomic>
#include <cstdint>
#include <vector>

namespace knect4
{

/** What a puck lands on: the board's base, deeper, or the puck below, brighter. */
enum class LandingOn
{
    kBase, ///< The bottom of the column.
    kPuck, ///< A puck already there.
};

/**
 * One click, as samples at `sample_rate`: a plastic tick - a burst of noise a fraction of a millisecond long, the
 * puck's own ringing a few kilohertz up and the board's lower knock beneath - gone within about a tenth of a second.
 * Each `seed` detunes and weighs it a little differently, so clicks in a row do not sound alike; the same seed and rate
 * give the same samples.
 * @param sample_rate The output's rate, in hertz.
 * @param landing What it lands on.
 * @param seed The variation.
 * @return The samples, mono, the loudest at 0.8.
 */
std::vector<float> RenderClick(int sample_rate, LandingOn landing, std::uint32_t seed);

/**
 * Clicks played on an audio thread, asked for from another: up to kVoices at once, each a copy of one of the sounds
 * set before the stream starts at a gain. Trigger is the asking thread's and Render the audio thread's; they meet only
 * in a queue of fixed size, with no lock and nothing allocated on the audio thread.
 */
class ClickMixer
{
  public:
    static constexpr int kVoices    = 8;  ///< Clicks sounding at once; another takes the oldest's place.
    static constexpr int kQueueSize = 32; ///< Clicks asked for and not yet started.

    /**
     * The sounds, before the stream starts (nothing renders while they are set): each a click's samples.
     * @param sounds The sounds, by number.
     */
    void SetSounds(std::vector<std::vector<float>> sounds);

    /** @return How many sounds are set. */
    int SoundCount() const noexcept
    {
        return static_cast<int>(sounds_.size());
    }

    /**
     * A click asked for: sound `sound` at `gain`, from the next block on. The asking thread's.
     * @param sound The sound's number; one outside the sounds is ignored.
     * @param gain Its gain, 0 to 1.
     * @return False when the queue was full and the click is dropped.
     */
    bool Trigger(int sound, float gain) noexcept;

    /**
     * One block: the clicks sounding, mixed into every channel. The audio thread's.
     * @param outputs Each channel's samples, written.
     * @param channels The channels.
     * @param frames The samples a channel.
     */
    void Render(float *const *outputs, int channels, int frames) noexcept;

    /** @return The clicks sounding after the last block. */
    int SoundingCount() const noexcept;

  private:
    /** A click asked for. */
    struct Request
    {
        int   sound{0};   ///< Which.
        float gain{0.0f}; ///< How loud.
    };

    /** A click sounding. */
    struct Voice
    {
        int   sound{-1};   ///< Which; -1 for none.
        int   position{0}; ///< The next sample.
        float gain{0.0f};  ///< How loud.
        int   age{0};      ///< When it started, counted in clicks.
    };

    std::vector<std::vector<float>> sounds_;      ///< The sounds.
    std::array<Request, kQueueSize> queue_{};     ///< The clicks asked for, a ring.
    std::atomic<unsigned>           written_{0};  ///< Requests written, by the asking thread.
    std::atomic<unsigned>           read_{0};     ///< Requests taken, by the audio thread.
    std::array<Voice, kVoices>      voices_{};    ///< The clicks sounding.
    int                             started_{0};  ///< Clicks started, for the voices' ages.
    std::atomic<int>                sounding_{0}; ///< The voices in use after the last block.
};

} // namespace knect4
