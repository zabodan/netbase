#pragma once
#include <boost/asio/ip/udp.hpp>

using boost::asio::ip::udp;


namespace core {

    class IConnection
    {
    public:

        virtual const udp::endpoint& peer() const = 0;
    };

}