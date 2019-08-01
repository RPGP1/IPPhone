#pragma once

#include "context.hpp"
#include "mainloop.hpp"

#include <pulse/stream.h>

#include <memory>
#include <optional>
#include <string>


namespace IPPhone
{

namespace PulseAudio
{

class StreamBase : public std::enable_shared_from_this<StreamBase>
{
protected:
    std::shared_ptr<Mainloop> m_mainloop;
    std::shared_ptr<pa_stream> m_stream;

    StreamBase(
        std::shared_ptr<Mainloop> const& mainloop,
        std::shared_ptr<Context> const& context,
        std::string const& name,
        pa_sample_spec const& sample_spec,
        std::optional<pa_channel_map> const& channel_map);

    StreamBase(StreamBase const&) = delete;
    StreamBase& operator=(StreamBase const&) & = delete;
    StreamBase(StreamBase&&) = delete;
    StreamBase& operator=(StreamBase&&) & = delete;

public:
    ~StreamBase() = default;

    pa_stream* get() const noexcept { return m_stream.get(); }
};

}  // namespace PulseAudio

}  // namespace IPPhone
