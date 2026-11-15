#pragma once

#include "hg/audio.hpp"

#include <memory>

#define WIN32_LEAN_AND_MEAN
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>

namespace hg {

class AudioDeviceWasapi : public AudioDevice {
private:
    IMMDeviceEnumerator* deviceEnumerator = nullptr;
    IMMDevice* device = nullptr;
    IAudioClient* audioClient = nullptr;
    IAudioRenderClient* renderClient = nullptr;
    HANDLE audioEvent = nullptr;
    HANDLE audioThread = nullptr;
    bool audioThreadRunning = false;
    u32 bufferFrameCount = 0;

    AudioCallback callback{};
    AudioConfig callbackConfig{};

    static DWORD WINAPI audioThreadProc(LPVOID param);

    void stopThread();
    void releaseStream();

public:
    static std::unique_ptr<AudioDeviceWasapi> create();
    ~AudioDeviceWasapi() override;

    virtual void setCallback(AudioCallback callback, const AudioConfig& preferredConfig = {}) override;
    virtual void unsetCallback() override;

    virtual AudioBackend getBackend() override
    {
        return AudioBackend::wasapi;
    }
};

} // namespace hg
