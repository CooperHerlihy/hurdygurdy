#pragma once

#include <pipewire/stream.h>
#include <pipewire/thread-loop.h>
#include <pipewire/context.h>
#include <pipewire/core.h>

extern "C" {
void pw_init(int* argc, char** argv[]);
void pw_deinit(void);
}

#define HG_PW_FUNC(name) decltype(&::name) name = nullptr

namespace hg::pipewire {

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

extern PipeWireFuncs pwFuncs;

bool loadPipeWire();

bool initPipewire();
void deinitPipewire();

} // namespace hg::pipewire
