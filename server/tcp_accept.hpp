#pragma once

#include <boost/asio.hpp>

#include "endpoint.hpp"


namespace IPPhone
{

namespace TCP
{

inline std::array<EndPoint, 2> get_me_and_accepted_endpoint(
    boost::asio::io_service& io_service,
    unsigned short port_num)
{
    using boost::asio::ip::tcp;


    tcp::acceptor accepter{io_service, tcp::endpoint{tcp::v4(), port_num}};
    tcp::socket socket{io_service};

    tcp::endpoint peer_endpoint;
    accepter.accept(socket, peer_endpoint);

    auto local_endpoint = socket.local_endpoint();

    return {{{local_endpoint.address(), local_endpoint.port()},
        {peer_endpoint.address(), peer_endpoint.port()}}};
}

}  // namespace TCP

}  // namespace IPPhone
