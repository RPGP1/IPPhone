#pragma once

#include <boost/asio.hpp>

#include "endpoint.hpp"


namespace IPPhone
{

namespace TCP
{

inline EndPoint get_local_connected_endpoint(
    boost::asio::io_service& io_service,
    EndPoint const& peer_endpoint)
{
    using boost::asio::ip::tcp;


    tcp::socket socket{io_service};
    socket.connect(static_cast<tcp::endpoint>(peer_endpoint));

    return {socket.local_endpoint().address(), socket.local_endpoint().port()};
}

}  // namespace TCP

}  // namespace IPPhone
