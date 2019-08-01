#pragma once

#include "sound.hpp"


namespace IPPhone
{

namespace Params
{

constexpr auto packet_header_size = 1 + sizeof(size_t);
constexpr auto packet_size = packet_header_size + fragment_size;

}  // namespace Params

}  // namespace IPPhone
