#include "playback_stream.hpp"

#include "exception.hpp"

// TODO delete
#include <iostream>


namespace IPPhone
{

namespace PulseAudio
{

PlaybackStream::PlaybackStream(
    std::shared_ptr<Mainloop> const& mainloop,
    std::shared_ptr<Context> const& context,
    std::string const& name,
    pa_sample_spec const& sample_spec,
    std::optional<pa_channel_map> const& channel_map)
    : StreamBase{mainloop, context, name, sample_spec, channel_map}
{
}

std::shared_ptr<PlaybackStream> PlaybackStream::create(
    std::shared_ptr<Mainloop> const& mainloop,
    std::shared_ptr<Context> const& context,
    std::string const& name,
    pa_sample_spec const& sample_spec,
    std::optional<pa_channel_map> const& channel_map)
{
    return std::shared_ptr<PlaybackStream>{new PlaybackStream{mainloop, context, name, sample_spec, channel_map}};
}

std::future<void> PlaybackStream::connect(
        std::optional<std::string> source,
        pa_buffer_attr buffer_attr,
        pa_stream_flags_t flag,
        std::optional<pa_cvolume> volume,
        std::shared_ptr<StreamBase> sync_stream) const
{
    using std::get;

    std::promise<void> promise;
    auto future = promise.get_future();

    std::thread{
        [mainloop = m_mainloop, stream = m_stream,
            source = std::move(source), buffer_attr = std::move(buffer_attr), flag,
            volume = std::move(volume), sync_stream = std::move(sync_stream),
            promise = std::move(promise)]() mutable {
            try {
                bool stream_connect_finished{false};
                std::mutex mutex{};
                std::condition_variable cond{};

                auto callback_data = std::tie(stream_connect_finished, mutex, cond);
                using callback_data_type = decltype(callback_data);

                {
                    auto mainloop_lock = mainloop->lock();

                    pa_stream_set_state_callback(
                        stream.get(),
                        [](pa_stream* _stream, void* data_ptr) {
                            auto stream_state = pa_stream_get_state(_stream);
                            if (!PA_STREAM_IS_GOOD(stream_state)
                                || stream_state == PA_STREAM_READY) {

                                decltype(auto) data = *static_cast<callback_data_type*>(data_ptr);

                                {
                                    std::lock_guard lock{get<std::mutex&>(data)};
                                    get<bool&>(data) = true;
                                }

                                get<std::condition_variable&>(data).notify_one();
                            }
                        },
                        &callback_data);

                    if (pa_stream_connect_playback(stream.get(), (source ? source->c_str() : nullptr), &buffer_attr, flag, (volume ? &volume.value() : nullptr), sync_stream ? sync_stream->get() : nullptr) != 0) {
                        throw IPPhone::PulseAudioError{"cannot initialize PulseAudio: pa_stream_connect_record"};
                    }
                }


                {
                    std::unique_lock lock{mutex};
                    cond.wait(lock, [&stream_connect_finished] {
                        return stream_connect_finished;
                    });

                    auto mainloop_lock = mainloop->lock();

                    pa_stream_set_state_callback(stream.get(), nullptr, nullptr);

                    if (!PA_STREAM_IS_GOOD(pa_stream_get_state(stream.get()))) {
                        throw IPPhone::PulseAudioError{"cannot initialize PulseAudio: pa_stream_connect_record"};
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

void PlaybackStream::set_write_callback(
    pa_stream_request_cb_t func,
    std::shared_ptr<void> userdata)
{
    m_write_callback_userdata = userdata;

    auto mainloop_lock = m_mainloop->lock();
    pa_stream_set_write_callback(this->get(),
        func,
        userdata.get());
}

void PlaybackStream::set_underflow_callback(
    pa_stream_notify_cb_t func,
    std::shared_ptr<void> userdata)
{
    m_write_callback_userdata = userdata;

    auto mainloop_lock = m_mainloop->lock();
    pa_stream_set_underflow_callback(this->get(),
        func,
        userdata.get());
}

}  // namespace PulseAudio

}  // namespace IPPhone
