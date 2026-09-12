#include "pipewire_platform.hpp"

#include "hg/error.hpp"
#include "hg/dynlib.hpp"
#include "hg/audio.hpp"

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

namespace hg::pipewire {

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
    Maybe<Library> lib = Library::load("libpipewire-0.3.so.0");
    if (!lib.has)
    {
        setError("Could not load libpipewire");
        return false;
    }
    libpipewire = std::move(*lib);

#define HG_LOAD_PW(name) \
    *(void**)&pwFuncs.name = libpipewire.findFunction(#name).orElse(nullptr); \
    if (pwFuncs.name == nullptr) { setError("Could not load " #name); return false; }

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
    struct pw_stream* stream = nullptr;
    struct spa_hook streamListener{};

    AudioCallback callback = nullptr;
    void* callbackData = nullptr;
    AudioConfig callbackConfig{};
};

static AudioState audio{};

static void streamProcess(void* userData)
{
    AudioState* state = static_cast<AudioState*>(userData);

    struct pw_buffer* pwBuf = pwFuncs.pw_stream_dequeue_buffer(state->stream);
    if (pwBuf == nullptr)
        return;

    struct spa_buffer* buf = pwBuf->buffer;
    f32* data = static_cast<f32*>(buf->datas[0].data);
    if (data == nullptr)
    {
        pwFuncs.pw_stream_queue_buffer(state->stream, pwBuf);
        return;
    }

    u64 maxBytes = buf->datas[0].maxsize;
    u64 frames = maxBytes / sizeof(f32) / state->callbackConfig.channels;
    u64 totalSamples = frames * state->callbackConfig.channels;
    memset(data, 0, totalSamples * sizeof(f32));
    Span<f32> buffer{data, totalSamples};

    if (state->callback != nullptr)
        state->callback(state->callbackData, buffer, state->callbackConfig);

    buf->datas[0].chunk->offset = 0;
    buf->datas[0].chunk->size = static_cast<uint32_t>(buffer.count * sizeof(f32));
    buf->datas[0].chunk->stride = static_cast<int32_t>(sizeof(f32) * state->callbackConfig.channels);
    pwBuf->size = static_cast<uint32_t>(frames);

    pwFuncs.pw_stream_queue_buffer(state->stream, pwBuf);
}

static void paramChanged(void* userData, uint32_t id, const struct spa_pod* param)
{
    AudioState* state = static_cast<AudioState*>(userData);
    if (id != SPA_PARAM_Format || param == nullptr)
        return;

    struct spa_audio_info_raw info{};
    if (spa_format_audio_raw_parse(param, &info) <= 0)
        return;

    state->callbackConfig.channels = info.channels;
    state->callbackConfig.sampleRate = info.rate;

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
        SPA_PARAM_BUFFERS_size, SPA_POD_Int(256 * static_cast<int>(sizeof(f32) * info.channels)),
        SPA_PARAM_BUFFERS_stride, SPA_POD_Int(static_cast<int>(sizeof(f32) * info.channels)),
        0);
    params[1] = spa_pod_builder_pop(&builder, &frame);

    pwFuncs.pw_stream_update_params(state->stream, params, 2);
}

static struct pw_stream_events streamEvents = {
    .version = PW_VERSION_STREAM_EVENTS,
    .destroy = nullptr,
    .state_changed = nullptr,
    .control_info = nullptr,
    .io_changed = nullptr,
    .param_changed = paramChanged,
    .add_buffer = nullptr,
    .remove_buffer = nullptr,
    .process = streamProcess,
    .drained = nullptr,
    .command = nullptr,
    .trigger_done = nullptr,
};

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

        if (audio.stream != nullptr)
        {
            pwFuncs.pw_stream_disconnect(audio.stream);
            pwFuncs.pw_stream_destroy(audio.stream);
            audio.stream = nullptr;
        }

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

void setAudioCallback(AudioCallback callback, void* userData, const AudioConfig& preferredConfig)
{
    if (audio.stream != nullptr)
    {
        pwFuncs.pw_stream_disconnect(audio.stream);
        pwFuncs.pw_stream_destroy(audio.stream);
        audio.stream = nullptr;
    }

    audio.callback = callback;
    audio.callbackData = userData;
    audio.callbackConfig = preferredConfig;

    if (audio.callbackConfig.sampleRate == 0)
        audio.callbackConfig.sampleRate = 48000;
    if (audio.callbackConfig.channels == 0)
        audio.callbackConfig.channels = 2;

    audio.stream = pwFuncs.pw_stream_new(audio.core, "hurdygurdy-audio", nullptr);
    if (audio.stream == nullptr)
    {
        setError("Could not create PipeWire stream");
        return;
    }

    uint8_t buffer[1024];
    struct spa_pod_builder builder;
    spa_pod_builder_init(&builder, buffer, sizeof(buffer));

    struct spa_audio_info_raw audioInfo{};
    audioInfo.format = SPA_AUDIO_FORMAT_F32;
    audioInfo.channels = audio.callbackConfig.channels;
    audioInfo.rate = audio.callbackConfig.sampleRate;

    const struct spa_pod* params[2];
    params[0] = spa_format_audio_raw_build(&builder, SPA_PARAM_EnumFormat, &audioInfo);

    struct spa_pod_frame frame;
    spa_pod_builder_push_object(&builder, &frame,
        SPA_TYPE_OBJECT_ParamBuffers, SPA_PARAM_Buffers);
    spa_pod_builder_add(&builder,
        SPA_PARAM_BUFFERS_buffers, SPA_POD_CHOICE_RANGE_Int(2, 1, 64),
        SPA_PARAM_BUFFERS_blocks, SPA_POD_Int(1),
        SPA_PARAM_BUFFERS_size, SPA_POD_Int(256 * static_cast<int>(sizeof(f32) * audio.callbackConfig.channels)),
        SPA_PARAM_BUFFERS_stride, SPA_POD_Int(static_cast<int>(sizeof(f32) * audio.callbackConfig.channels)),
        0);
    params[1] = spa_pod_builder_pop(&builder, &frame);

    pwFuncs.pw_thread_loop_lock(audio.threadLoop);

    pwFuncs.pw_stream_add_listener(audio.stream, &audio.streamListener, &streamEvents, &audio);

    pwFuncs.pw_stream_connect(
        audio.stream,
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

    pwFuncs.pw_thread_loop_unlock(audio.threadLoop);

    pwFuncs.pw_thread_loop_start(audio.threadLoop);
}

void unsetAudioCallback()
{
    audio.callback = nullptr;
    audio.callbackData = nullptr;
    audio.callbackConfig = {};

    if (audio.stream != nullptr)
    {
        pwFuncs.pw_thread_loop_lock(audio.threadLoop);
        pwFuncs.pw_stream_disconnect(audio.stream);
        pwFuncs.pw_stream_destroy(audio.stream);
        pwFuncs.pw_thread_loop_unlock(audio.threadLoop);
        audio.stream = nullptr;
    }
}

} // namespace hg::pipewire
