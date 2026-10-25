#include "app/click_player.h"

#include "DeviceBackends.h"

#include <algorithm>
#include <utility>
#include <vector>

namespace knect4::app
{

ClickPlayer::ClickPlayer(SystemMaker make_system) : make_system_(std::move(make_system))
{
    if (!make_system_)
    {
        make_system_ = [] {
            const std::string api = reckless::audio::DefaultAudioApi();
            return api.empty() ? nullptr : reckless::audio::MakeAudioSystem(api);
        };
    }
}

ClickPlayer::~ClickPlayer()
{
    Close();
}

void ClickPlayer::Prepare()
{
    quiet_for_ = 0.0;
    if (is_running_ || retry_in_ > 0.0)
    {
        return;
    }
    if (!Open())
    {
        retry_in_ = kRetrySeconds;
    }
}

bool ClickPlayer::Open()
{
    Close();
    system_ = make_system_();
    if (!system_)
    {
        error_ = "no audio API";
        return false;
    }
    const std::vector<reckless::audio::DeviceInfo> devices = system_->Devices();
    const auto output = std::find_if(devices.begin(), devices.end(), [](const reckless::audio::DeviceInfo &device) {
        return device.is_default_output && device.output_channels > 0;
    });
    const auto any = std::find_if(devices.begin(), devices.end(),
                                  [](const reckless::audio::DeviceInfo &device) { return device.output_channels > 0; });
    const auto chosen = output != devices.end() ? output : any;
    if (chosen == devices.end())
    {
        error_ = system_->Name() + ": no output device";
        system_.reset();
        return false;
    }
    reckless::audio::StreamRequest request;
    request.output_device = chosen->name;
    request.output_count  = std::min(chosen->output_channels, 2);
    request.sample_rate   = chosen->DefaultSampleRate();
    request.block_size    = 256;
    request.client_name   = "Knect 4";
    has_failed_.store(false);
    if (!system_->Open(request, *this))
    {
        error_ = system_->Name() + ": " + system_->LastError();
        system_.reset();
        return false;
    }
    // The clicks at the rate the stream opened at, before it starts.
    const int                       rate = system_->Format().sample_rate;
    std::vector<std::vector<float>> sounds;
    for (const LandingOn landing : {LandingOn::kBase, LandingOn::kPuck})
    {
        for (int seed = 0; seed != kVariations; ++seed)
        {
            sounds.push_back(RenderClick(rate, landing, static_cast<std::uint32_t>(seed)));
        }
    }
    mixer_.SetSounds(std::move(sounds));
    if (!system_->Start())
    {
        error_ = system_->Name() + ": " + system_->LastError();
        system_->Close();
        system_.reset();
        return false;
    }
    error_.clear();
    is_running_ = true;
    return true;
}

void ClickPlayer::Close()
{
    if (system_)
    {
        system_->Close();
        system_.reset();
    }
    is_running_ = false;
}

void ClickPlayer::Frame(double seconds)
{
    retry_in_ = std::max(retry_in_ - seconds, 0.0);
    if (!is_running_)
    {
        return;
    }
    if (has_failed_.load())
    {
        Close();
        error_ = "the output stopped";
        return;
    }
    quiet_for_ = mixer_.SoundingCount() > 0 ? 0.0 : quiet_for_ + seconds;
    if (quiet_for_ >= kIdleSeconds)
    {
        Close();
    }
}

void ClickPlayer::Click(LandingOn landing, float gain)
{
    quiet_for_ = 0.0;
    if (!is_running_)
    {
        return;
    }
    const int first = landing == LandingOn::kBase ? 0 : kVariations;
    mixer_.Trigger(first + static_cast<int>(next_++ % kVariations), gain);
}

void ClickPlayer::Process(const float *const *inputs, int input_channels, float *const *outputs, int output_channels,
                          int frames, std::int64_t now, bool xrun) noexcept
{
    static_cast<void>(inputs);
    static_cast<void>(input_channels);
    static_cast<void>(now);
    static_cast<void>(xrun);
    mixer_.Render(outputs, output_channels, frames);
}

void ClickPlayer::StreamFailed() noexcept
{
    has_failed_.store(true);
}

} // namespace knect4::app
