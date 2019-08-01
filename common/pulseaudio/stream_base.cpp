#include "stream_base.hpp"

#include "exception.hpp"


namespace IPPhone
{

namespace PulseAudio
{

StreamBase::StreamBase(
    std::shared_ptr<Mainloop> const& mainloop,
    std::shared_ptr<Context> const& context,
    std::string const& name,
    pa_sample_spec const& sample_spec,
    std::optional<pa_channel_map> const& channel_map)
    : m_mainloop{mainloop}
{
    {
        auto mainloop_lock = m_mainloop->lock();

        m_stream.reset(
            pa_stream_new(context->get(), name.c_str(), &sample_spec, channel_map ? &channel_map.value() : nullptr),
            [mainloop](pa_stream* ptr) {
                if (mainloop) {
                    auto lock = mainloop->lock();

                    if (PA_STREAM_IS_GOOD(pa_stream_get_state(ptr))) {
                        pa_stream_disconnect(ptr);
                    }
                }
                pa_stream_unref(ptr);
            });
    }

    if (!m_stream) {
        throw IPPhone::PulseAudioError{"cannot initialize PulseAudio: pa_stream_new"};
    }
}

}  // namespace PulseAudio

}  // namespace IPPhone
