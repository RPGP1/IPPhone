#pragma once

#include "mainloop.hpp"

#include <pulse/context.h>

#include <future>
#include <memory>
#include <optional>
#include <string>


namespace IPPhone
{

namespace PulseAudio
{

class Context final : public std::enable_shared_from_this<Context>
{
    std::shared_ptr<Mainloop> m_mainloop;
    std::shared_ptr<pa_context> m_context;

    Context(
        std::shared_ptr<Mainloop> const& mainloop,
        std::string const& name);

    Context(Context const&) = delete;
    Context& operator=(Context const&) & = delete;
    Context(Context&&) = delete;
    Context& operator=(Context&&) & = delete;

public:
    static std::shared_ptr<Context> create(
        std::shared_ptr<Mainloop> const& mainloop,
        std::string const& name);

    ~Context() = default;

    std::future<void> connect(
        std::optional<std::string> server,
        pa_context_flags_t flag,
        pa_spawn_api const* spawn_api) const;

    pa_context* get() const noexcept { return m_context.get(); }
};

}  // namespace PulseAudio

}  // namespace IPPhone
