#include "context.hpp"

#include "pulseaudio/exception.hpp"

#include <pulse/error.h>

#include <sstream>


namespace IPPhone
{

namespace PulseAudio
{

Context::Context(
    std::shared_ptr<Mainloop> const& mainloop,
    std::string const& name)
    : m_mainloop{mainloop}
{
    {
        auto mainloop_lock = m_mainloop->lock();

        m_context.reset(
            pa_context_new(m_mainloop->api(), name.c_str()),
            [mainloop](pa_context* ptr) {
                if (mainloop) {
                    auto lock = mainloop->lock();

                    if (PA_CONTEXT_IS_GOOD(pa_context_get_state(ptr))) {
                        pa_context_disconnect(ptr);
                    }
                }
                pa_context_unref(ptr);
            });
    }

    if (!m_context) {
        throw IPPhone::PulseAudioError{"cannot initialize PulseAudio: pa_context_new"};
    }
}

std::shared_ptr<Context> Context::create(
    std::shared_ptr<Mainloop> const& mainloop,
    std::string const& name)
{
    return std::shared_ptr<Context>{new Context{mainloop, name}};
}

std::future<void> Context::connect(
    std::optional<std::string> server,
    pa_context_flags_t flag,
    pa_spawn_api const* spawn_api) const
{
    using std::get;

    std::promise<void> promise;
    auto future = promise.get_future();

    std::thread{
        [mainloop = m_mainloop, context = m_context,
            server = std::move(server), flag, spawn_api,
            promise = std::move(promise)]() mutable {
            try {
                bool context_connect_finished{false};
                std::mutex mutex{};
                std::condition_variable cond{};

                auto callback_data = std::tie(context_connect_finished, mutex, cond);
                using callback_data_type = decltype(callback_data);

                {
                    auto mainloop_lock = mainloop->lock();

                    pa_context_set_state_callback(
                        context.get(),
                        [](pa_context* _context, void* data_ptr) {
                            auto context_state = pa_context_get_state(_context);
                            if (!PA_CONTEXT_IS_GOOD(context_state)
                                || context_state == PA_CONTEXT_READY) {

                                decltype(auto) data = *static_cast<callback_data_type*>(data_ptr);

                                {
                                    std::lock_guard lock{get<std::mutex&>(data)};
                                    get<bool&>(data) = true;
                                }

                                get<std::condition_variable&>(data).notify_one();
                            }
                        },
                        &callback_data);

                    if (pa_context_connect(context.get(), (server ? server->c_str() : nullptr), flag, spawn_api) < 0) {
                        std::ostringstream osstr;
                        osstr << "cannot initialize PulseAudio: pa_context_connect; "
                              << pa_strerror(pa_context_errno(context.get()));

                        throw IPPhone::PulseAudioError{osstr.str()};
                    }
                }


                {
                    std::unique_lock lock{mutex};
                    cond.wait(lock, [&context_connect_finished] {
                        return context_connect_finished;
                    });

                    auto mainloop_lock = mainloop->lock();

                    pa_context_set_state_callback(context.get(), nullptr, nullptr);

                    if (!PA_CONTEXT_IS_GOOD(pa_context_get_state(context.get()))) {
                        std::ostringstream osstr;
                        osstr << "cannot initialize PulseAudio: pa_context_connect; "
                              << pa_strerror(pa_context_errno(context.get()));

                        throw IPPhone::PulseAudioError{osstr.str()};
                    }
                }

                try {
                    promise.set_value();

                } catch (std::future_error& err) {
                    if (err.code() != std::make_error_condition(std::future_errc::no_state)) {
                        throw;
                    }
                }

            } catch (...) {
                promise.set_exception(std::current_exception());
            }
        }}
        .detach();

    return future;
}

}  // namespace PulseAudio

}  // namespace IPPhone
