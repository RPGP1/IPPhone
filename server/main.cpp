#include "endpoint.hpp"
#include "tcp_accept.hpp"

#include "pulseaudio/context.hpp"
#include "pulseaudio/exception.hpp"
#include "pulseaudio/mainloop.hpp"
#include "pulseaudio/playback_stream.hpp"
#include "pulseaudio/record_stream.hpp"

#include "params/communication.hpp"
#include "params/sound.hpp"


#include <cereal/archives/portable_binary.hpp>

#include <pulse/pulseaudio.h>

#include <boost/asio.hpp>


#include <algorithm>
#include <future>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>


namespace asio = boost::asio;
using asio::ip::tcp;
using asio::ip::udp;


std::atomic<bool> program_finished{false};

int main(int argc, char** argv)
{
    std::signal(SIGINT, [](int) { program_finished = true; });

    auto mainloop = IPPhone::PulseAudio::Mainloop::instance();

    auto context = IPPhone::PulseAudio::Context::create(mainloop, "IPPhone_server");

    using StreamPairType = std::tuple<std::shared_ptr<IPPhone::PulseAudio::RecordStream>, std::shared_ptr<IPPhone::PulseAudio::PlaybackStream>>;
    std::future<StreamPairType> future_stream_create;
    {
        std::promise<StreamPairType> promise_stream_create;
        future_stream_create = promise_stream_create.get_future();

        std::thread{
            [mainloop, context,
                promise_stream_create = std::move(promise_stream_create),
                future_context_connect = context->connect(std::nullopt, PA_CONTEXT_NOFLAGS, nullptr)]() mutable {
                try {

                    future_context_connect.wait();

                    auto record_stream = IPPhone::PulseAudio::RecordStream::create(mainloop, context, "Record", IPPhone::Params::sample_spec, std::nullopt);
                    auto playback_stream = IPPhone::PulseAudio::PlaybackStream::create(mainloop, context, "Playback", IPPhone::Params::sample_spec, std::nullopt);

                    promise_stream_create.set_value_at_thread_exit({record_stream, playback_stream});

                } catch (...) {
                    promise_stream_create.set_exception_at_thread_exit(std::current_exception());
                }
            }}
            .detach();
    }


    unsigned short port_num;
    {
        std::istringstream port_sstr{argv[1]};

        if (argc != 2 || !(port_sstr >> port_num) || port_sstr.good()) {
            std::ostringstream err_sstr;
            err_sstr << "Usage: " << argv[0] << " PORT_NUMBER";
            throw std::invalid_argument{err_sstr.str()};
        }
    }

    asio::io_service io_service;
    std::optional<asio::io_service::work> work{io_service};
    std::thread asio_thread{[&io_service] {
        io_service.run();
    }};


    auto endpoints = IPPhone::TCP::get_me_and_accepted_endpoint(io_service, port_num);
    decltype(auto) local_endpoint = endpoints.at(0);
    decltype(auto) peer_endpoint = endpoints.at(1);

    // create, open, bind
    auto socket = std::make_shared<udp::socket>(io_service, static_cast<udp::endpoint>(local_endpoint));
    socket->connect(static_cast<udp::endpoint>(peer_endpoint));
    auto strand = std::make_shared<asio::io_service::strand>(io_service);
    std::cerr << "Local: " << socket->local_endpoint().address().to_string()
              << " (" << socket->local_endpoint().port() << ")" << std::endl
              << "Peer: " << socket->remote_endpoint().address().to_string()
              << " (" << socket->remote_endpoint().port() << ")" << std::endl;


    std::shared_ptr<IPPhone::PulseAudio::RecordStream> record_stream;
    std::shared_ptr<IPPhone::PulseAudio::PlaybackStream> playback_stream;
    std::tie(record_stream, playback_stream) = future_stream_create.get();

    std::cerr << "connected" << std::endl;


    using CallbackDataType = std::tuple<std::shared_ptr<udp::socket>, std::shared_ptr<asio::io_service::strand>>;
    record_stream->set_read_callback(
        [](pa_stream* _stream, size_t prepared_size, void* callback_data_ptr) {
            using std::get;

            const char* data_buffer;

            if (pa_stream_peek(_stream, reinterpret_cast<const void**>(&data_buffer), &prepared_size) < 0) {
                throw IPPhone::PulseAudioError{"cannot read recorded data: pa_stream_peek"};
            }

            if (!data_buffer) {
                if (prepared_size != 0) {
                    pa_stream_drop(_stream);  // drop buffer hole
                }
                return;
            }

            if (prepared_size > 0) {
                std::ostringstream to_be_sent{std::ios::binary};
                {
                    cereal::PortableBinaryOutputArchive ar{to_be_sent};

                    ar(prepared_size);
                    ar(cereal::binary_data(&(*data_buffer), prepared_size));
                }


                decltype(auto) callback_data = *static_cast<CallbackDataType*>(callback_data_ptr);
                decltype(auto) socket = get<std::shared_ptr<udp::socket>>(callback_data);
                decltype(auto) strand = get<std::shared_ptr<asio::io_service::strand>>(callback_data);
                strand->get_io_service().post(
                    strand->wrap(
                        [socket, to_be_sent = to_be_sent.str()]() {
                            socket->async_send(
                                asio::buffer(to_be_sent),
                                [](boost::system::error_code const& error, size_t) {
                                    if (error) {
                                        if (error == boost::system::errc::connection_refused) {
                                            program_finished = true;
                                        }
                                        std::cerr << "async_send: " << error.message() << std::endl;
                                    }
                                });
                        }));
            }

            pa_stream_drop(_stream);
        },
        std::make_shared<CallbackDataType>(socket, strand));

    record_stream->connect(std::nullopt, IPPhone::Params::record_buffer_attr, PA_STREAM_ADJUST_LATENCY).wait();
    playback_stream->connect(std::nullopt, IPPhone::Params::playback_buffer_attr, PA_STREAM_ADJUST_LATENCY, std::nullopt, nullptr).wait();

    std::function<void(boost::system::error_code const&, size_t)> functor;
    std::array<char, IPPhone::Params::packet_size> receive_buffer;
    functor = [mainloop, playback_stream, strand, socket, functer_ref = std::ref(functor), buffer_ref = std::ref(receive_buffer)](boost::system::error_code const& error, size_t size) {
        if (error == boost::system::errc::connection_refused) {
            program_finished = true;
        }

        if (!program_finished) {
            strand->get_io_service().post(
                strand->wrap([socket, functer_ref, buffer_ref]() {
                    socket->async_receive(asio::buffer(buffer_ref.get()), functer_ref.get());
                }));
        }

        if (error) {
            std::cerr << "async_receive: " << error.message() << std::endl;

            return;
        }

        if (size <= IPPhone::Params::packet_header_size) {
            return;
        }

        std::istringstream to_be_played{std::string{buffer_ref.get().data(), size}, std::ios::binary};

        cereal::PortableBinaryInputArchive ar{to_be_played};

        size_t data_size;
        ar(data_size);

        auto mainloop_lock = mainloop->lock();

        if ((data_size = std::min({data_size, pa_stream_writable_size(playback_stream->get())})) <= 0) {
            return;
        }

        void* data_buffer;

        if (pa_stream_begin_write(playback_stream->get(), &data_buffer, &data_size) != 0 || !data_buffer) {
            throw IPPhone::PulseAudioError{"cannot prepare playback buffer: pa_stream_begin_write"};
        } else if (data_size == 0) {
            pa_stream_cancel_write(playback_stream->get());
            return;
        }

        ar(cereal::binary_data(data_buffer, data_size));

        pa_stream_write(playback_stream->get(), data_buffer, data_size, nullptr, 0, PA_SEEK_RELATIVE);
    };
    socket->async_receive(asio::buffer(receive_buffer), functor);


    std::thread finish_keeper{[work = std::move(work)] {
        while (!program_finished) {
            std::this_thread::sleep_for(std::chrono::milliseconds{10});
        }
    }};
    work.reset();
    finish_keeper.join();
    asio_thread.join();
}
