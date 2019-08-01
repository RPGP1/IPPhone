#include "mainloop.hpp"

#include "exception.hpp"


namespace IPPhone
{

namespace PulseAudio
{

Mainloop::Mainloop()
    : m_mainloop{
          pa_threaded_mainloop_new(),
          [](pa_threaded_mainloop* ptr) {
              pa_threaded_mainloop_stop(ptr);
              pa_threaded_mainloop_free(ptr);
          }},
      m_mainloop_api{std::shared_ptr<pa_mainloop_api>{nullptr}, pa_threaded_mainloop_get_api(m_mainloop.get())}
{
    if (!m_mainloop) {
        throw IPPhone::PulseAudioError{"cannot initialize PulseAudio: pa_threaded_mainloop_new"};
    }

    if (pa_threaded_mainloop_start(this->get()) != 0) {
        throw IPPhone::PulseAudioError{"cannot initialize PulseAudio: pa_threaded_mainloop_start"};
    }
}

std::shared_ptr<Mainloop> Mainloop::instance()
{
    static std::shared_ptr<Mainloop> mainloop{new Mainloop};
    return mainloop;
}

Mainloop::MainloopLock Mainloop::lock()
{
    return {shared_from_this()};
}

Mainloop::MainloopLock::MainloopLock(std::shared_ptr<Mainloop> const& mainloop) noexcept
    : m_mainloop{mainloop}
{
    if (m_mainloop) {
        pa_threaded_mainloop_lock(m_mainloop->get());
    }
}
Mainloop::MainloopLock::~MainloopLock() noexcept
{
    if (m_mainloop) {
        pa_threaded_mainloop_unlock(m_mainloop->get());
    }
}

}  // namespace PulseAudio

}  // namespace IPPhone
