#include "win32_platform.hpp"

#include "hg/error.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <audiopolicy.h>

#include <string.h>

namespace hg::win32 {

struct AudioState {
    IMMDeviceEnumerator* deviceEnumerator = nullptr;
    IMMDevice* device = nullptr;
    IAudioClient* audioClient = nullptr;
    IAudioRenderClient* renderClient = nullptr;
    HANDLE audioThread = nullptr;
    HANDLE audioEvent = nullptr;
    bool audioThreadRunning = false;
    u32 bufferFrameCount = 0;

    AudioCallback callback = nullptr;
    void* callbackData = nullptr;
    AudioConfig callbackConfig{};
};

static AudioState audio{};

static DWORD WINAPI wasapiAudioThread(LPVOID lpParam)
{
    (void)lpParam;

    CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    while (audio.audioThreadRunning)
    {
        DWORD waitResult = WaitForSingleObject(audio.audioEvent, 1000);
        if (waitResult == WAIT_TIMEOUT)
            continue;
        if (!audio.audioThreadRunning)
            break;

        u32 padding = 0;
        HRESULT hr = audio.audioClient->GetCurrentPadding(&padding);
        if (FAILED(hr))
            continue;

        u32 availableFrames = audio.bufferFrameCount - padding;
        if (availableFrames == 0)
            continue;

        BYTE* data = nullptr;
        hr = audio.renderClient->GetBuffer(availableFrames, &data);
        if (FAILED(hr))
            continue;

        memset(data, 0, static_cast<u64>(availableFrames) * sizeof(f32) * audio.callbackConfig.channels);

        if (audio.callback != nullptr)
        {
            u64 totalSamples = static_cast<u64>(availableFrames) * audio.callbackConfig.channels;
            Span<f32> buffer{reinterpret_cast<f32*>(data), totalSamples};
            audio.callback(audio.callbackData, buffer, audio.callbackConfig);
        }

        audio.renderClient->ReleaseBuffer(availableFrames, 0);
    }

    CoUninitialize();
    return 0;
}

bool initAudio()
{
    // Initialize COM on the main thread (per-thread requirement)
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    HRESULT hr = CoCreateInstance(
        __uuidof(MMDeviceEnumerator),
        nullptr,
        CLSCTX_ALL,
        __uuidof(IMMDeviceEnumerator),
        reinterpret_cast<void**>(&audio.deviceEnumerator));

    if (FAILED(hr) || audio.deviceEnumerator == nullptr)
    {
        setError("Could not create WASAPI device enumerator");
        return false;
    }

    hr = audio.deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &audio.device);
    if (FAILED(hr) || audio.device == nullptr)
    {
        setError("Could not get default audio endpoint");
        audio.deviceEnumerator->Release();
        audio.deviceEnumerator = nullptr;
        return false;
    }

    hr = audio.device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, reinterpret_cast<void**>(&audio.audioClient));
    if (FAILED(hr) || audio.audioClient == nullptr)
    {
        setError("Could not activate audio client");
        audio.device->Release();
        audio.device = nullptr;
        audio.deviceEnumerator->Release();
        audio.deviceEnumerator = nullptr;
        return false;
    }

    return true;
}

void deinitAudio()
{
    if (audio.audioThreadRunning)
    {
        audio.audioThreadRunning = false;
        if (audio.audioEvent != nullptr)
            SetEvent(audio.audioEvent);
        if (audio.audioThread != nullptr)
        {
            WaitForSingleObject(audio.audioThread, 5000);
            CloseHandle(audio.audioThread);
            audio.audioThread = nullptr;
        }
    }

    if (audio.audioEvent != nullptr)
    {
        CloseHandle(audio.audioEvent);
        audio.audioEvent = nullptr;
    }

    if (audio.renderClient != nullptr)
    {
        audio.renderClient->Release();
        audio.renderClient = nullptr;
    }

    if (audio.audioClient != nullptr)
    {
        audio.audioClient->Release();
        audio.audioClient = nullptr;
    }

    if (audio.device != nullptr)
    {
        audio.device->Release();
        audio.device = nullptr;
    }

    if (audio.deviceEnumerator != nullptr)
    {
        audio.deviceEnumerator->Release();
        audio.deviceEnumerator = nullptr;
    }

    // Uninitialize COM on the main thread
    CoUninitialize();
}

void setAudioCallback(AudioCallback callback, void* userData, const AudioConfig& preferredConfig)
{
    if (audio.audioThreadRunning)
    {
        audio.audioThreadRunning = false;
        if (audio.audioEvent != nullptr)
            SetEvent(audio.audioEvent);
        if (audio.audioThread != nullptr)
        {
            WaitForSingleObject(audio.audioThread, 5000);
            CloseHandle(audio.audioThread);
            audio.audioThread = nullptr;
        }
    }

    if (audio.renderClient != nullptr)
    {
        audio.renderClient->Release();
        audio.renderClient = nullptr;
    }

    audio.callback = callback;
    audio.callbackData = userData;
    audio.callbackConfig = preferredConfig;

    if (audio.callbackConfig.sampleRate == 0)
        audio.callbackConfig.sampleRate = 48000;
    if (audio.callbackConfig.channels == 0)
        audio.callbackConfig.channels = 2;

    // Get default format from the audio client
    WAVEFORMATEX* mixFormat = nullptr;
    HRESULT hr = audio.audioClient->GetMixFormat(&mixFormat);
    if (FAILED(hr) || mixFormat == nullptr)
    {
        setError("Could not get audio mix format");
        return;
    }

    // In shared mode, use the mix format directly
    // Update callback config to match what the system provides
    audio.callbackConfig.sampleRate = static_cast<u32>(mixFormat->nSamplesPerSec);
    audio.callbackConfig.channels = static_cast<u32>(mixFormat->nChannels);

    // Set event handle for event-driven mode
    audio.audioEvent = CreateEventA(nullptr, FALSE, FALSE, nullptr);
    if (audio.audioEvent == nullptr)
    {
        setError("Could not create audio event");
        CoTaskMemFree(mixFormat);
        return;
    }

    // Initialize in shared mode with event callback, using mix format
    hr = audio.audioClient->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
        0,
        0,
        mixFormat,
        nullptr);

    CoTaskMemFree(mixFormat);

    if (FAILED(hr))
    {
        setError("Could not initialize audio client");
        CloseHandle(audio.audioEvent);
        audio.audioEvent = nullptr;
        return;
    }

    audio.audioClient->SetEventHandle(audio.audioEvent);

    // Get buffer size
    hr = audio.audioClient->GetBufferSize(&audio.bufferFrameCount);
    if (FAILED(hr))
    {
        setError("Could not get audio buffer size");
        audio.audioClient->Release();
        audio.audioClient = nullptr;
        CloseHandle(audio.audioEvent);
        audio.audioEvent = nullptr;
        return;
    }

    // Get render client
    hr = audio.audioClient->GetService(__uuidof(IAudioRenderClient), reinterpret_cast<void**>(&audio.renderClient));
    if (FAILED(hr) || audio.renderClient == nullptr)
    {
        setError("Could not get audio render client");
        audio.audioClient->Release();
        audio.audioClient = nullptr;
        CloseHandle(audio.audioEvent);
        audio.audioEvent = nullptr;
        return;
    }

    // Start the audio client
    hr = audio.audioClient->Start();
    if (FAILED(hr))
    {
        setError("Could not start audio client");
        audio.renderClient->Release();
        audio.renderClient = nullptr;
        audio.audioClient->Release();
        audio.audioClient = nullptr;
        CloseHandle(audio.audioEvent);
        audio.audioEvent = nullptr;
        return;
    }

    // Start audio thread
    audio.audioThreadRunning = true;
    audio.audioThread = CreateThread(nullptr, 0, wasapiAudioThread, nullptr, 0, nullptr);
    if (audio.audioThread == nullptr)
    {
        setError("Could not create audio thread");
        audio.audioClient->Stop();
        audio.renderClient->Release();
        audio.renderClient = nullptr;
        audio.audioClient->Release();
        audio.audioClient = nullptr;
        CloseHandle(audio.audioEvent);
        audio.audioEvent = nullptr;
        audio.audioThreadRunning = false;
        return;
    }
}

void unsetAudioCallback()
{
    audio.callback = nullptr;
    audio.callbackData = nullptr;
    audio.callbackConfig = {};

    if (audio.audioThreadRunning)
    {
        audio.audioThreadRunning = false;
        if (audio.audioEvent != nullptr)
            SetEvent(audio.audioEvent);
        if (audio.audioThread != nullptr)
        {
            WaitForSingleObject(audio.audioThread, 5000);
            CloseHandle(audio.audioThread);
            audio.audioThread = nullptr;
        }
    }

    if (audio.audioClient != nullptr)
        audio.audioClient->Stop();

    if (audio.renderClient != nullptr)
    {
        audio.renderClient->Release();
        audio.renderClient = nullptr;
    }

    if (audio.audioClient != nullptr)
    {
        audio.audioClient->Release();
        audio.audioClient = nullptr;
    }

    if (audio.audioEvent != nullptr)
    {
        CloseHandle(audio.audioEvent);
        audio.audioEvent = nullptr;
    }
}

} // namespace hg::win32
