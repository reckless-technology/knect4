#include "effects/click_sound.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <utility>

namespace connect4
{

namespace
{

/** A ringing part of the click: a sine dying away. */
struct Mode
{
    double frequency; ///< In hertz.
    double decay;     ///< The time it takes to fall to 1/e, in seconds.
    double level;     ///< Its loudness, before the whole is scaled.
};

/** The puck's own ringing: plastic a few kilohertz up, gone in a few milliseconds. */
constexpr std::array<Mode, 4> kPuckModes{{
    {2350.0, 0.0075, 0.55},
    {3720.0, 0.0050, 0.38},
    {5480.0, 0.0032, 0.22},
    {8100.0, 0.0018, 0.12},
}};

/** The board's knock beneath, lower and longer. */
constexpr std::array<Mode, 2> kBoardModes{{
    {360.0, 0.022, 0.30},
    {170.0, 0.035, 0.18},
}};

/** A small generator of numbers, the same from the same seed on every platform. */
class Noise
{
  public:
    /** @param seed Where it starts. */
    explicit Noise(std::uint32_t seed) : state_(seed * 2654435761u + 0x9e3779b9u)
    {
    }

    /** @return The next number, from -1 to 1. */
    double Next() noexcept
    {
        // xorshift32
        state_ ^= state_ << 13;
        state_ ^= state_ >> 17;
        state_ ^= state_ << 5;
        return static_cast<double>(state_) / 2147483648.0 - 1.0;
    }

  private:
    std::uint32_t state_; ///< Never 0.
};

} // namespace

std::vector<float> RenderClick(int sample_rate, LandingOn landing, std::uint32_t seed)
{
    const int          rate   = std::max(sample_rate, 8000);
    const double       length = 0.15;
    const int          count  = static_cast<int>(length * rate);
    std::vector<float> samples(static_cast<std::size_t>(count), 0.0f);

    Noise        noise(seed + 1u);
    const double detune      = 1.0 + 0.05 * noise.Next(); // the whole click a little higher or lower
    const bool   is_on_base  = landing == LandingOn::kBase;
    const double puck_pitch  = detune * (is_on_base ? 0.88 : 1.0);
    const double board_level = is_on_base ? 1.6 : 0.8;
    std::array<double, kPuckModes.size()>  puck_levels{};
    std::array<double, kBoardModes.size()> board_levels{};
    for (std::size_t index = 0; index < kPuckModes.size(); ++index)
    {
        puck_levels[index] = kPuckModes[index].level * (1.0 + 0.12 * noise.Next());
    }
    for (std::size_t index = 0; index < kBoardModes.size(); ++index)
    {
        board_levels[index] = kBoardModes[index].level * board_level * (1.0 + 0.12 * noise.Next());
    }

    const double        two_pi   = 2.0 * std::numbers::pi;
    double              previous = 0.0; // the noise's last sample, for its high-pass
    double              peak     = 0.0;
    std::vector<double> mixed(static_cast<std::size_t>(count), 0.0);
    for (int index = 0; index < count; ++index)
    {
        const double t = static_cast<double>(index) / rate;
        // The tick: noise a fraction of a millisecond long, its lows taken off by a difference.
        const double burst = noise.Next() * std::exp(-t / 0.0006);
        double       value = 0.6 * (burst - previous);
        previous           = burst;
        for (std::size_t mode = 0; mode < kPuckModes.size(); ++mode)
        {
            const Mode &ringing = kPuckModes[mode];
            value += puck_levels[mode] * std::sin(two_pi * ringing.frequency * puck_pitch * t) *
                     std::exp(-t / ringing.decay);
        }
        // The knock rises over a millisecond rather than starting on a step, which would thump.
        const double onset = 1.0 - std::exp(-t / 0.0008);
        for (std::size_t mode = 0; mode < kBoardModes.size(); ++mode)
        {
            const Mode &ringing = kBoardModes[mode];
            value += onset * board_levels[mode] * std::sin(two_pi * ringing.frequency * detune * t) *
                     std::exp(-t / ringing.decay);
        }
        // The last 10 ms faded to nothing, so the end is silent whatever is left.
        const double to_end = static_cast<double>(count - 1 - index) / rate;
        if (to_end < 0.01)
        {
            value *= std::max(to_end / 0.01, 0.0);
        }
        mixed[static_cast<std::size_t>(index)] = value;
        peak                                   = std::max(peak, std::abs(value));
    }
    const double scale = peak > 0.0 ? 0.8 / peak : 0.0;
    for (int index = 0; index < count; ++index)
    {
        samples[static_cast<std::size_t>(index)] = static_cast<float>(mixed[static_cast<std::size_t>(index)] * scale);
    }
    return samples;
}

void ClickMixer::SetSounds(std::vector<std::vector<float>> sounds)
{
    sounds_ = std::move(sounds);
    for (Voice &voice : voices_)
    {
        voice.sound = -1;
    }
    read_.store(written_.load(std::memory_order_acquire), std::memory_order_release);
}

bool ClickMixer::Trigger(int sound, float gain) noexcept
{
    if (sound < 0 || sound >= SoundCount())
    {
        return false;
    }
    const unsigned written = written_.load(std::memory_order_relaxed);
    if (written - read_.load(std::memory_order_acquire) >= static_cast<unsigned>(kQueueSize))
    {
        return false;
    }
    queue_[written % kQueueSize] = {sound, std::clamp(gain, 0.0f, 1.0f)};
    written_.store(written + 1, std::memory_order_release);
    return true;
}

void ClickMixer::Render(float *const *outputs, int channels, int frames) noexcept
{
    for (int channel = 0; channel < channels; ++channel)
    {
        std::fill(outputs[channel], outputs[channel] + frames, 0.0f);
    }
    // The clicks asked for, each into a free voice or the oldest's.
    const unsigned written = written_.load(std::memory_order_acquire);
    unsigned       read    = read_.load(std::memory_order_relaxed);
    while (read != written)
    {
        const Request request = queue_[read % kQueueSize];
        ++read;
        Voice *chosen = &voices_[0];
        for (Voice &voice : voices_)
        {
            if (voice.sound < 0)
            {
                chosen = &voice;
                break;
            }
            if (voice.age < chosen->age)
            {
                chosen = &voice;
            }
        }
        *chosen = {request.sound, 0, request.gain, started_++};
    }
    read_.store(read, std::memory_order_release);

    int sounding = 0;
    for (Voice &voice : voices_)
    {
        if (voice.sound < 0)
        {
            continue;
        }
        const std::vector<float> &sound = sounds_[static_cast<std::size_t>(voice.sound)];
        const int                 left  = static_cast<int>(sound.size()) - voice.position;
        const int                 count = std::min(left, frames);
        for (int frame = 0; frame < count; ++frame)
        {
            const float sample = sound[static_cast<std::size_t>(voice.position + frame)] * voice.gain;
            for (int channel = 0; channel < channels; ++channel)
            {
                outputs[channel][frame] += sample;
            }
        }
        voice.position += count;
        if (voice.position >= static_cast<int>(sound.size()))
        {
            voice.sound = -1;
        }
        else
        {
            ++sounding;
        }
    }
    // Clicks that pile up held to full scale rather than wrapping in the device's integers.
    for (int channel = 0; channel < channels; ++channel)
    {
        for (int frame = 0; frame < frames; ++frame)
        {
            outputs[channel][frame] = std::clamp(outputs[channel][frame], -1.0f, 1.0f);
        }
    }
    sounding_.store(sounding, std::memory_order_relaxed);
}

int ClickMixer::SoundingCount() const noexcept
{
    return sounding_.load(std::memory_order_relaxed);
}

} // namespace connect4
