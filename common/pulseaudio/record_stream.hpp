#pragma once

#include "stream_base.hpp"

#include <future>


namespace IPPhone
{

namespace PulseAudio
{

class RecordStream : public StreamBase
{
protected:
    std::shared_ptr<void> m_read_callback_userdata;

    RecordStream(
        std::shared_ptr<Mainloop> const& mainloop,
        std::shared_ptr<Context> const& context,
        std::string const& name,
        pa_sample_spec const& sample_spec,
        std::optional<pa_channel_map> const& channel_map);

    RecordStream(RecordStream const&) = delete;
    RecordStream& operator=(RecordStream const&) & = delete;
    RecordStream(RecordStream&&) = delete;
    RecordStream& operator=(RecordStream&&) & = delete;

public:
    static std::shared_ptr<RecordStream> create(
        std::shared_ptr<Mainloop> const& mainloop,
        std::shared_ptr<Context> const& context,
        std::string const& name,
        pa_sample_spec const& sample_spec,
        std::optional<pa_channel_map> const& channel_map);

    ~RecordStream() { m_stream.reset(); }

    std::future<void> connect(
        std::optional<std::string> source,
        pa_buffer_attr buffer_attr,
        pa_stream_flags_t flag) const;

    void set_read_callback(
        pa_stream_request_cb_t func,
        std::shared_ptr<void> userdata);

    pa_stream* get() const noexcept { return m_stream.get(); }
};

}  // namespace PulseAudio

}  // namespace IPPhone
