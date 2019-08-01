#pragma once

#include <pulse/thread-mainloop.h>

#include <memory>


namespace IPPhone
{

namespace PulseAudio
{

class Mainloop final : public std::enable_shared_from_this<Mainloop>
{
    std::shared_ptr<pa_threaded_mainloop> m_mainloop;
    std::shared_ptr<pa_mainloop_api> m_mainloop_api;

    Mainloop();

    Mainloop(Mainloop const&) = delete;
    Mainloop& operator=(Mainloop const&) & = delete;
    Mainloop(Mainloop&&) = delete;
    Mainloop& operator=(Mainloop&&) & = delete;

public:
    static std::shared_ptr<Mainloop> instance();
    ~Mainloop() = default;

    pa_threaded_mainloop* get() const noexcept { return m_mainloop.get(); }
    pa_mainloop_api* api() const noexcept { return m_mainloop_api.get(); }

private:
    class MainloopLock final
    {
        std::shared_ptr<Mainloop> m_mainloop;

    public:
        MainloopLock(std::shared_ptr<Mainloop> const& mainloop) noexcept;
        ~MainloopLock() noexcept;
    };

public:
    MainloopLock lock();
};

}  // namespace PulseAudio

}  // namespace IPPhone
