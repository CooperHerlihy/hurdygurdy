#pragma once

#include "hg/audio.hpp"

#include <pipewire/stream.h>
#include <pipewire/thread-loop.h>
#include <pipewire/context.h>
#include <pipewire/core.h>

#include <spa/param/audio/format-utils.h>

#include <memory>

namespace hg {

class AudioDevicePipewire : public AudioDevice {
private:
    struct pw_stream* stream = nullptr;
    struct spa_hook streamListener{};

    AudioCallback callback{};
    AudioConfig callbackConfig{};

    static struct pw_stream_events streamEvents;

    static void streamProcess(void* userData);
    static void paramChanged(void* userData, uint32_t id, const struct spa_pod* param);

public:
    static std::unique_ptr<AudioDevicePipewire> create();
    ~AudioDevicePipewire() override;

    virtual void setCallback(AudioCallback callback, const AudioConfig& preferredConfig = {}) override;
    virtual void unsetCallback() override;

    virtual AudioBackend getBackend() override
    {
        return AudioBackend::pipewire;
    }
};

} // namespace hg
