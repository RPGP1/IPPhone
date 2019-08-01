#pragma once

#include <stdexcept>

namespace IPPhone
{

struct PulseAudioError : std::runtime_error {
    PulseAudioError(std::string const& message)
        : std::runtime_error{message}
    {
    }
};

}  // namespace IPPhone
