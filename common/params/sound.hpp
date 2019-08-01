#pragma once

#include <pulse/sample.h>
#include <pulse/def.h>

#include <limits>


namespace IPPhone
{

namespace Params
{

constexpr pa_sample_spec sample_spec{
    .format = PA_SAMPLE_S16LE,
    .rate = 44100,
    .channels = 1};

constexpr auto frame_size = [](pa_sample_spec const& _spec) {
    return sizeof(int16_t) * _spec.channels;
}(sample_spec);

constexpr auto fragment_size = [](pa_sample_spec const& _spec, decltype(frame_size) const& _frame_size) {
    return _spec.rate / 100 * _frame_size;
}(sample_spec, frame_size);

constexpr auto record_buffer_attr = [](decltype(fragment_size) const& _fragment_size) {
    pa_buffer_attr tmp{};
    tmp.maxlength = std::numeric_limits<decltype(pa_buffer_attr::maxlength)>::max();
    tmp.fragsize = _fragment_size;

    return tmp;
}(fragment_size);

constexpr auto playback_buffer_attr = [](decltype(fragment_size) const& _fragment_size) {
    pa_buffer_attr tmp{};
    tmp.maxlength = _fragment_size * 20;
    tmp.tlength = _fragment_size;
    tmp.prebuf = tmp.tlength * 10;
    tmp.minreq = tmp.maxlength;

    return tmp;
}(fragment_size);


}  // namespace Params

}  // namespace IPPhone
