#include "pipewire_audio.hpp"

#include "hg/utility.hpp"
#include "hg/dynlib.hpp"
#include "hg/utility.hpp"

extern "C" {
void pw_init(int* argc, char** argv[]);
void pw_deinit(void);
}

#include <pipewire/stream.h>
#include <pipewire/thread-loop.h>
#include <pipewire/context.h>
#include <pipewire/core.h>
#include <spa/param/audio/format-utils.h>
#include <spa/param/audio/raw.h>

namespace hg {

namespace pipewire {

#define HG_PW_FUNC(name) decltype(&::name) name = nullptr

struct PipeWireFuncs {
    HG_PW_FUNC(pw_init);
    HG_PW_FUNC(pw_deinit);
    HG_PW_FUNC(pw_thread_loop_new);
    HG_PW_FUNC(pw_thread_loop_destroy);
    HG_PW_FUNC(pw_thread_loop_start);
    HG_PW_FUNC(pw_thread_loop_stop);
    HG_PW_FUNC(pw_thread_loop_lock);
    HG_PW_FUNC(pw_thread_loop_unlock);
    HG_PW_FUNC(pw_thread_loop_get_loop);
    HG_PW_FUNC(pw_context_new);
    HG_PW_FUNC(pw_context_connect);
    HG_PW_FUNC(pw_context_destroy);
    HG_PW_FUNC(pw_core_disconnect);
    HG_PW_FUNC(pw_stream_new);
    HG_PW_FUNC(pw_stream_destroy);
    HG_PW_FUNC(pw_stream_connect);
    HG_PW_FUNC(pw_stream_disconnect);
    HG_PW_FUNC(pw_stream_dequeue_buffer);
    HG_PW_FUNC(pw_stream_queue_buffer);
    HG_PW_FUNC(pw_stream_add_listener);
    HG_PW_FUNC(pw_stream_update_params);
};

#undef HG_PW_FUNC

static PipeWireFuncs pwFuncs{};
static Library libpipewire{};

bool loadPipeWire()
{
    std::optional<Library> lib = Library::load("libpipewire-0.3.so.0");
    if (!lib.has_value())
    {
        setError("Could not load libpipewire");
        return false;
    }
    libpipewire = std::move(*lib);

#define HG_LOAD_PW(name) \
    { \
        auto sym = libpipewire.loadSymbol(#name); \
        if (!sym.has_value()) \
        { \
            setError("Could not load " #name); \
            return false; \
        } \
        *(void**)&pwFuncs.name = *sym; \
    }

    HG_LOAD_PW(pw_init);
    HG_LOAD_PW(pw_deinit);
    HG_LOAD_PW(pw_thread_loop_new);
    HG_LOAD_PW(pw_thread_loop_destroy);
    HG_LOAD_PW(pw_thread_loop_start);
    HG_LOAD_PW(pw_thread_loop_stop);
    HG_LOAD_PW(pw_thread_loop_lock);
    HG_LOAD_PW(pw_thread_loop_unlock);
    HG_LOAD_PW(pw_thread_loop_get_loop);
    HG_LOAD_PW(pw_context_new);
    HG_LOAD_PW(pw_context_connect);
    HG_LOAD_PW(pw_context_destroy);
    HG_LOAD_PW(pw_core_disconnect);
    HG_LOAD_PW(pw_stream_new);
    HG_LOAD_PW(pw_stream_destroy);
    HG_LOAD_PW(pw_stream_connect);
    HG_LOAD_PW(pw_stream_disconnect);
    HG_LOAD_PW(pw_stream_dequeue_buffer);
    HG_LOAD_PW(pw_stream_queue_buffer);
    HG_LOAD_PW(pw_stream_add_listener);
    HG_LOAD_PW(pw_stream_update_params);

#undef HG_LOAD_PW

    return true;
}

struct AudioState {
    struct pw_thread_loop* threadLoop = nullptr;
    struct pw_context* context = nullptr;
    struct pw_core* core = nullptr;
};

static AudioState audio{};

bool initPipewire()
{
    pwFuncs.pw_init(nullptr, nullptr);

    audio.threadLoop = pwFuncs.pw_thread_loop_new("hurdygurdy", nullptr);
    if (audio.threadLoop == nullptr)
    {
        setError("Could not create PipeWire thread loop");
        return false;
    }

    struct pw_loop* loop = pwFuncs.pw_thread_loop_get_loop(audio.threadLoop);
    audio.context = pwFuncs.pw_context_new(loop, nullptr, 0);
    if (audio.context == nullptr)
    {
        setError("Could not create PipeWire context");
        pwFuncs.pw_thread_loop_destroy(audio.threadLoop);
        return false;
    }

    audio.core = pwFuncs.pw_context_connect(audio.context, nullptr, 0);
    if (audio.core == nullptr)
    {
        setError("Could not connect to PipeWire");
        pwFuncs.pw_context_destroy(audio.context);
        pwFuncs.pw_thread_loop_destroy(audio.threadLoop);
        return false;
    }

    return true;
}

void deinitPipewire()
{
    if (audio.threadLoop != nullptr)
    {
        pwFuncs.pw_thread_loop_lock(audio.threadLoop);

        if (audio.core != nullptr)
        {
            pwFuncs.pw_core_disconnect(audio.core);
            audio.core = nullptr;
        }

        if (audio.context != nullptr)
        {
            pwFuncs.pw_context_destroy(audio.context);
            audio.context = nullptr;
        }

        pwFuncs.pw_thread_loop_unlock(audio.threadLoop);
        pwFuncs.pw_thread_loop_stop(audio.threadLoop);
        pwFuncs.pw_thread_loop_destroy(audio.threadLoop);
        audio.threadLoop = nullptr;
    }

    pwFuncs.pw_deinit();
}

} // namespace pipewire

struct pw_stream_events AudioDevicePipewire::streamEvents = {
    .version = PW_VERSION_STREAM_EVENTS,
    .destroy = nullptr,
    .state_changed = nullptr,
    .control_info = nullptr,
    .io_changed = nullptr,
    .param_changed = AudioDevicePipewire::paramChanged,
    .add_buffer = nullptr,
    .remove_buffer = nullptr,
    .process = AudioDevicePipewire::streamProcess,
    .drained = nullptr,
    .command = nullptr,
    .trigger_done = nullptr,
};

void AudioDevicePipewire::streamProcess(void* userData)
{
    AudioDevicePipewire* audio = static_cast<AudioDevicePipewire*>(userData);

    struct pw_buffer* pwBuf = pipewire::pwFuncs.pw_stream_dequeue_buffer(audio->stream);
    if (pwBuf == nullptr)
        return;

    struct spa_buffer* buf = pwBuf->buffer;
    f32* data = static_cast<f32*>(buf->datas[0].data);
    if (data == nullptr)
    {
        pipewire::pwFuncs.pw_stream_queue_buffer(audio->stream, pwBuf);
        return;
    }

    u64 maxBytes = buf->datas[0].maxsize;
    u64 frames = maxBytes / sizeof(f32) / audio->callbackConfig.channels;
    u64 totalSamples = frames * audio->callbackConfig.channels;
    std::memset(data, 0, totalSamples * sizeof(f32));
    std::span<f32> buffer{data, totalSamples};

    if (audio->callback != nullptr)
        audio->callback(buffer, audio->callbackConfig);

    buf->datas[0].chunk->offset = 0;
    buf->datas[0].chunk->size = assertCast<uint32_t>(buffer.size() * sizeof(f32));
    buf->datas[0].chunk->stride = assertCast<int32_t>(sizeof(f32) * audio->callbackConfig.channels);
    pwBuf->size = assertCast<uint32_t>(frames);

    pipewire::pwFuncs.pw_stream_queue_buffer(audio->stream, pwBuf);
}

void AudioDevicePipewire::paramChanged(void* userData, uint32_t id, const struct spa_pod* param)
{
    AudioDevicePipewire* audio = static_cast<AudioDevicePipewire*>(userData);
    if (id != SPA_PARAM_Format || param == nullptr)
        return;

    struct spa_audio_info_raw info{};
    if (spa_format_audio_raw_parse(param, &info) <= 0)
        return;

    audio->callbackConfig.channels = info.channels;
    audio->callbackConfig.sampleRate = info.rate;

    uint8_t buffer[1024];
    struct spa_pod_builder builder;
    spa_pod_builder_init(&builder, buffer, sizeof(buffer));

    const struct spa_pod* params[2];
    params[0] = spa_format_audio_raw_build(&builder, SPA_PARAM_Format, &info);

    struct spa_pod_frame frame;
    spa_pod_builder_push_object(&builder, &frame,
        SPA_TYPE_OBJECT_ParamBuffers, SPA_PARAM_Buffers);
    spa_pod_builder_add(&builder,
        SPA_PARAM_BUFFERS_buffers, SPA_POD_CHOICE_RANGE_Int(2, 1, 64),
        SPA_PARAM_BUFFERS_blocks, SPA_POD_Int(1),
        SPA_PARAM_BUFFERS_size, SPA_POD_Int(256 * assertCast<int>(sizeof(f32) * info.channels)),
        SPA_PARAM_BUFFERS_stride, SPA_POD_Int(assertCast<int>(sizeof(f32) * info.channels)),
        0);
    params[1] = spa_pod_builder_pop(&builder, &frame);

    pipewire::pwFuncs.pw_stream_update_params(audio->stream, params, 2);
}

std::unique_ptr<AudioDevicePipewire> AudioDevicePipewire::create()
{
    if (pipewire::audio.threadLoop == nullptr)
    {
        if (!pipewire::loadPipeWire())
            return nullptr;

        if (!pipewire::initPipewire())
            return nullptr;
    }

    return std::make_unique<AudioDevicePipewire>();
}

AudioDevicePipewire::~AudioDevicePipewire()
{
    unsetCallback();
    pipewire::deinitPipewire();
}

void AudioDevicePipewire::setCallback(AudioCallback callback, const AudioConfig& preferredConfig)
{
    if (stream != nullptr)
    {
        pipewire::pwFuncs.pw_thread_loop_lock(pipewire::audio.threadLoop);
        HG_DEFER(pipewire::pwFuncs.pw_thread_loop_unlock(pipewire::audio.threadLoop));

        pipewire::pwFuncs.pw_stream_disconnect(stream);
        pipewire::pwFuncs.pw_stream_destroy(stream);
        stream = nullptr;
    }

    this->callback = std::move(callback);

    callbackConfig = preferredConfig;
    if (callbackConfig.sampleRate == 0)
        callbackConfig.sampleRate = 48000;
    if (callbackConfig.channels == 0)
        callbackConfig.channels = 2;

    stream = pipewire::pwFuncs.pw_stream_new(pipewire::audio.core, "hurdygurdy-audio", nullptr);
    if (stream == nullptr)
    {
        setError("Could not create PipeWire stream");
        return;
    }

    uint8_t buffer[1024];
    struct spa_pod_builder builder;
    spa_pod_builder_init(&builder, buffer, sizeof(buffer));

    struct spa_audio_info_raw audioInfo{};
    audioInfo.format = SPA_AUDIO_FORMAT_F32;
    audioInfo.channels = callbackConfig.channels;
    audioInfo.rate = callbackConfig.sampleRate;

    const struct spa_pod* params[2];
    params[0] = spa_format_audio_raw_build(&builder, SPA_PARAM_EnumFormat, &audioInfo);

    struct spa_pod_frame frame;
    spa_pod_builder_push_object(&builder, &frame,
        SPA_TYPE_OBJECT_ParamBuffers, SPA_PARAM_Buffers);
    spa_pod_builder_add(&builder,
        SPA_PARAM_BUFFERS_buffers, SPA_POD_CHOICE_RANGE_Int(2, 1, 64),
        SPA_PARAM_BUFFERS_blocks, SPA_POD_Int(1),
        SPA_PARAM_BUFFERS_size, SPA_POD_Int(256 * assertCast<int>(sizeof(f32) * callbackConfig.channels)),
        SPA_PARAM_BUFFERS_stride, SPA_POD_Int(assertCast<int>(sizeof(f32) * callbackConfig.channels)),
        0);
    params[1] = spa_pod_builder_pop(&builder, &frame);

    {
        pipewire::pwFuncs.pw_thread_loop_lock(pipewire::audio.threadLoop);
        HG_DEFER(pipewire::pwFuncs.pw_thread_loop_unlock(pipewire::audio.threadLoop));

        pipewire::pwFuncs.pw_stream_add_listener(stream, &streamListener, &streamEvents, this);

        pipewire::pwFuncs.pw_stream_connect(
            stream,
            SPA_DIRECTION_OUTPUT,
            PW_ID_ANY,
            static_cast<pw_stream_flags>(
                PW_STREAM_FLAG_AUTOCONNECT |
                PW_STREAM_FLAG_MAP_BUFFERS |
                PW_STREAM_FLAG_RT_PROCESS
            ),
            params,
            2
        );
    }

    pipewire::pwFuncs.pw_thread_loop_start(pipewire::audio.threadLoop);
}

void AudioDevicePipewire::unsetCallback()
{
    callback = {};
    callbackConfig = {};

    if (stream != nullptr)
    {
        pipewire::pwFuncs.pw_thread_loop_lock(pipewire::audio.threadLoop);
        HG_DEFER(pipewire::pwFuncs.pw_thread_loop_unlock(pipewire::audio.threadLoop));

        pipewire::pwFuncs.pw_stream_disconnect(stream);
        pipewire::pwFuncs.pw_stream_destroy(stream);
        stream = nullptr;
    }
}

} // namespace hg
