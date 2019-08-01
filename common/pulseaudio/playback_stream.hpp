#pragma once

#include "stream_base.hpp"

#include <future>


namespace IPPhone
{

namespace PulseAudio
{

class PlaybackStream : public StreamBase
{
protected:
    std::shared_ptr<void> m_write_callback_userdata;
    std::shared_ptr<void> m_underflow_callback_userdata;

    PlaybackStream(
        std::shared_ptr<Mainloop> const& mainloop,
        std::shared_ptr<Context> const& context,
        std::string const& name,
        pa_sample_spec const& sample_spec,
        std::optional<pa_channel_map> const& channel_map);

    PlaybackStream(PlaybackStream const&) = delete;
    PlaybackStream& operator=(PlaybackStream const&) & = delete;
    PlaybackStream(PlaybackStream&&) = delete;
    PlaybackStream& operator=(PlaybackStream&&) & = delete;

public:
    static std::shared_ptr<PlaybackStream> create(
        std::shared_ptr<Mainloop> const& mainloop,
        std::shared_ptr<Context> const& context,
        std::string const& name,
        pa_sample_spec const& sample_spec,
        std::optional<pa_channel_map> const& channel_map);

    ~PlaybackStream() { m_stream.reset(); }

    std::future<void> connect(
        std::optional<std::string> source,
        pa_buffer_attr buffer_attr,
        pa_stream_flags_t flag,
        std::optional<pa_cvolume> volume,
        std::shared_ptr<StreamBase> sync_stream) const;

    void set_write_callback(
        pa_stream_request_cb_t func,
        std::shared_ptr<void> userdata);

    void set_underflow_callback(
        pa_stream_notify_cb_t func,
        std::shared_ptr<void> userdata);

    pa_stream* get() const noexcept { return m_stream.get(); }
};

}  // namespace PulseAudio

}  // namespace IPPhone
