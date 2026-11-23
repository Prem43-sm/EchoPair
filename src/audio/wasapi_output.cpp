#include <echopair/audio/wasapi_output.h>

#include "../platform/com_support.h"

#include <windows.h>
#include <ks.h>
#include <ksmedia.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <wrl/client.h>

#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <exception>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace echopair::audio {
namespace {

using Microsoft::WRL::ComPtr;
using echopair::platform::check_hresult;
using echopair::platform::ComApartment;

constexpr double pi = 3.14159265358979323846;
constexpr double tone_frequency_hz = 440.0;
constexpr double tone_amplitude = 0.15;
constexpr REFERENCE_TIME requested_buffer_duration = 1'000'000;

struct WaveFormatDeleter {
    void operator()(WAVEFORMATEX* format) const noexcept
    {
        CoTaskMemFree(format);
    }
};

struct HandleCloser {
    void operator()(void* handle) const noexcept
    {
        if (handle != nullptr) {
            CloseHandle(handle);
        }
    }
};

using WaveFormatPtr = std::unique_ptr<WAVEFORMATEX, WaveFormatDeleter>;
using EventHandle = std::unique_ptr<void, HandleCloser>;

struct FormatInfo {
    bool is_float;
    std::uint16_t channels;
    std::uint16_t container_bits;
    std::uint16_t valid_bits;
    std::uint32_t sample_rate;
    std::uint16_t block_alignment;
};

FormatInfo inspect_format(const WAVEFORMATEX& format)
{
    bool is_float = format.wFormatTag == WAVE_FORMAT_IEEE_FLOAT;
    std::uint16_t valid_bits = format.wBitsPerSample;

    if (format.wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
        constexpr auto extensible_size =
            static_cast<WORD>(sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX));
        if (format.cbSize < extensible_size) {
            throw std::runtime_error("Endpoint returned a truncated extensible audio format");
        }

        const auto& extensible = reinterpret_cast<const WAVEFORMATEXTENSIBLE&>(format);
        is_float = IsEqualGUID(extensible.SubFormat, KSDATAFORMAT_SUBTYPE_IEEE_FLOAT) != FALSE;
        if (!is_float &&
            IsEqualGUID(extensible.SubFormat, KSDATAFORMAT_SUBTYPE_PCM) == FALSE) {
            throw std::runtime_error("Endpoint uses an unsupported shared-mode audio format");
        }
        valid_bits = extensible.Samples.wValidBitsPerSample;
    } else if (format.wFormatTag != WAVE_FORMAT_PCM && !is_float) {
        throw std::runtime_error("Endpoint uses an unsupported shared-mode audio format");
    }

    if (format.nChannels == 0 || format.nSamplesPerSec == 0 ||
        format.wBitsPerSample == 0 || format.wBitsPerSample % 8 != 0 ||
        valid_bits == 0 || valid_bits > format.wBitsPerSample) {
        throw std::runtime_error("Endpoint returned invalid audio format dimensions");
    }

    if ((is_float && (format.wBitsPerSample != 32 || valid_bits != 32)) ||
        (!is_float && format.wBitsPerSample != 8 &&
         format.wBitsPerSample != 16 && format.wBitsPerSample != 24 &&
         format.wBitsPerSample != 32)) {
        throw std::runtime_error("Endpoint sample depth is unsupported by the test-tone renderer");
    }

    const auto expected_alignment = static_cast<std::uint32_t>(format.nChannels) *
                                   (format.wBitsPerSample / 8);
    if (expected_alignment != format.nBlockAlign) {
        throw std::runtime_error("Endpoint returned an inconsistent audio frame size");
    }

    return {
        is_float,
        format.nChannels,
        format.wBitsPerSample,
        valid_bits,
        format.nSamplesPerSec,
        format.nBlockAlign
    };
}

void write_sample(BYTE* destination, double value, const FormatInfo& format)
{
    if (format.is_float) {
        const float sample = static_cast<float>(value);
        std::memcpy(destination, &sample, sizeof(sample));
        return;
    }

    const std::int64_t scale = std::int64_t{1} << (format.valid_bits - 1);
    const std::int64_t signed_sample =
        static_cast<std::int64_t>(std::llround(value * static_cast<double>(scale - 1)));

    if (format.container_bits == 8) {
        const std::uint8_t sample = static_cast<std::uint8_t>(
            (signed_sample + scale) << (8 - format.valid_bits));
        *destination = sample;
        return;
    }

    const std::int64_t left_aligned_sample =
        signed_sample * (std::int64_t{1} << (format.container_bits - format.valid_bits));
    const auto encoded_sample = static_cast<std::uint32_t>(left_aligned_sample);
    for (std::uint16_t byte = 0; byte < format.container_bits / 8; ++byte) {
        destination[byte] = static_cast<BYTE>(encoded_sample >> (byte * 8));
    }
}

void fill_tone(
    BYTE* buffer,
    UINT32 frames,
    const FormatInfo& format,
    double& phase)
{
    const double phase_increment = 2.0 * pi * tone_frequency_hz / format.sample_rate;
    const std::uint16_t bytes_per_sample = format.container_bits / 8;

    for (UINT32 frame = 0; frame < frames; ++frame) {
        const double value = tone_amplitude * std::sin(phase);
        phase += phase_increment;
        if (phase >= 2.0 * pi) {
            phase -= 2.0 * pi;
        }

        BYTE* frame_data = buffer + static_cast<std::size_t>(frame) * format.block_alignment;
        for (std::uint16_t channel = 0; channel < format.channels; ++channel) {
            write_sample(frame_data + static_cast<std::size_t>(channel) * bytes_per_sample,
                         value, format);
        }
    }
}

class AudioClientStart final {
public:
    explicit AudioClientStart(IAudioClient* client) noexcept : client_(client) {}

    ~AudioClientStart()
    {
        if (started_) {
            client_->Stop();
        }
    }

    void start()
    {
        check_hresult(client_->Start(), "IAudioClient::Start");
        started_ = true;
    }

    void stop()
    {
        check_hresult(client_->Stop(), "IAudioClient::Stop");
        started_ = false;
    }

    AudioClientStart(const AudioClientStart&) = delete;
    AudioClientStart& operator=(const AudioClientStart&) = delete;

private:
    IAudioClient* client_;
    bool started_ = false;
};

void render_packet(
    IAudioClient* audio_client,
    IAudioRenderClient* render_client,
    UINT32 buffer_frames,
    const FormatInfo& format,
    double& phase)
{
    UINT32 padding = 0;
    check_hresult(audio_client->GetCurrentPadding(&padding),
                  "IAudioClient::GetCurrentPadding");
    if (padding > buffer_frames) {
        throw std::runtime_error("Audio endpoint reported padding larger than its buffer");
    }

    const UINT32 frames_available = buffer_frames - padding;
    if (frames_available == 0) {
        return;
    }

    BYTE* data = nullptr;
    check_hresult(render_client->GetBuffer(frames_available, &data),
                  "IAudioRenderClient::GetBuffer");
    fill_tone(data, frames_available, format, phase);
    check_hresult(render_client->ReleaseBuffer(frames_available, 0),
                  "IAudioRenderClient::ReleaseBuffer");
}

}

void play_test_tone_on_endpoint(
    const std::wstring& endpoint_id,
    std::chrono::milliseconds duration)
{
    if (endpoint_id.empty()) {
        throw std::invalid_argument("An audio endpoint identifier is required");
    }
    if (duration <= std::chrono::milliseconds::zero()) {
        throw std::invalid_argument("Test-tone duration must be positive");
    }

    ComApartment apartment;
    ComPtr<IMMDeviceEnumerator> enumerator;
    check_hresult(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                   IID_PPV_ARGS(enumerator.GetAddressOf())),
                  "CoCreateInstance(MMDeviceEnumerator)");

    ComPtr<IMMDevice> device;
    check_hresult(enumerator->GetDevice(endpoint_id.c_str(), device.GetAddressOf()),
                  "IMMDeviceEnumerator::GetDevice");

    ComPtr<IAudioClient> audio_client;
    check_hresult(device->Activate(
                      __uuidof(IAudioClient),
                      CLSCTX_ALL,
                      nullptr,
                      reinterpret_cast<void**>(audio_client.GetAddressOf())),
                  "IMMDevice::Activate(IAudioClient)");

    WAVEFORMATEX* raw_format = nullptr;
    check_hresult(audio_client->GetMixFormat(&raw_format), "IAudioClient::GetMixFormat");
    const WaveFormatPtr wave_format(raw_format);
    const FormatInfo format = inspect_format(*wave_format);

    check_hresult(audio_client->Initialize(
                      AUDCLNT_SHAREMODE_SHARED,
                      AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
                      requested_buffer_duration,
                      0,
                      wave_format.get(),
                      nullptr),
                  "IAudioClient::Initialize");

    EventHandle event(CreateEventW(nullptr, FALSE, FALSE, nullptr));
    if (!event) {
        throw std::runtime_error("CreateEventW failed for the audio render stream");
    }
    check_hresult(audio_client->SetEventHandle(static_cast<HANDLE>(event.get())),
                  "IAudioClient::SetEventHandle");

    UINT32 buffer_frames = 0;
    check_hresult(audio_client->GetBufferSize(&buffer_frames),
                  "IAudioClient::GetBufferSize");

    ComPtr<IAudioRenderClient> render_client;
    check_hresult(audio_client->GetService(IID_PPV_ARGS(render_client.GetAddressOf())),
                  "IAudioClient::GetService(IAudioRenderClient)");

    double phase = 0.0;
    BYTE* initial_data = nullptr;
    check_hresult(render_client->GetBuffer(buffer_frames, &initial_data),
                  "IAudioRenderClient::GetBuffer(initial)");
    fill_tone(initial_data, buffer_frames, format, phase);
    check_hresult(render_client->ReleaseBuffer(buffer_frames, 0),
                  "IAudioRenderClient::ReleaseBuffer(initial)");

    AudioClientStart stream(audio_client.Get());
    stream.start();

    const auto stop_time = std::chrono::steady_clock::now() + duration;
    while (std::chrono::steady_clock::now() < stop_time) {
        const DWORD wait_result = WaitForSingleObject(static_cast<HANDLE>(event.get()), 1000);
        if (wait_result == WAIT_FAILED) {
            std::ostringstream message;
            message << "WaitForSingleObject failed (Win32 error " << GetLastError() << ')';
            throw std::runtime_error(message.str());
        }
        if (wait_result == WAIT_OBJECT_0) {
            render_packet(audio_client.Get(), render_client.Get(), buffer_frames, format, phase);
        } else if (wait_result != WAIT_TIMEOUT) {
            throw std::runtime_error("Audio render event returned an unexpected wait result");
        }
    }

    stream.stop();
}

void play_test_tone_on_endpoints(
    const std::vector<std::wstring>& endpoint_ids,
    std::chrono::milliseconds duration)
{
    if (endpoint_ids.empty() || endpoint_ids.size() > 2) {
        throw std::invalid_argument("Select one or two playback endpoints");
    }
    if (duration <= std::chrono::milliseconds::zero()) {
        throw std::invalid_argument("Test-tone duration must be positive");
    }

    for (std::size_t first = 0; first < endpoint_ids.size(); ++first) {
        if (endpoint_ids[first].empty()) {
            throw std::invalid_argument("An audio endpoint identifier is required");
        }
        for (std::size_t second = first + 1; second < endpoint_ids.size(); ++second) {
            if (endpoint_ids[first] == endpoint_ids[second]) {
                throw std::invalid_argument("Select two different playback endpoints");
            }
        }
    }

    std::vector<std::exception_ptr> failures(endpoint_ids.size());
    std::vector<std::jthread> workers;
    workers.reserve(endpoint_ids.size());
    for (std::size_t index = 0; index < endpoint_ids.size(); ++index) {
        workers.emplace_back([&, index] {
            try {
                play_test_tone_on_endpoint(endpoint_ids[index], duration);
            } catch (...) {
                failures[index] = std::current_exception();
            }
        });
    }
    workers.clear();

    for (const auto& failure : failures) {
        if (failure) {
            std::rethrow_exception(failure);
        }
    }
}

}
