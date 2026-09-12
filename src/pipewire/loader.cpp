#include "pipewire_internal.hpp"

#include "hg/error.hpp"
#include "hg/dynlib.hpp"

namespace hg::pipewire {

static Library libpipewire{};
PipeWireFuncs pwFuncs{};

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

} // namespace hg::pipewire
