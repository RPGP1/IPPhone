#pragma once

#include <boost/asio.hpp>

namespace IPPhone
{

struct EndPoint {
    boost::asio::ip::address address;
    unsigned short port;

    explicit operator boost::asio::ip::tcp::endpoint() const
    {
        return {address, port};
    }
    explicit operator boost::asio::ip::udp::endpoint() const
    {
        return {address, port};
    }
};

}  // namespace IPPhone
