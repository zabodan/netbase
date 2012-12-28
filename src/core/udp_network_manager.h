#pragma once
#include "core/udp_connection.h"
#include "core/udp_packet_dispatcher.h"
#include <map>


namespace core
{

    typedef std::shared_ptr<boost::asio::io_service> IOServicePtr;


    class UdpNetworkManager
    {
    public:

        UdpNetworkManager(const IOServicePtr& ioservice, size_t port);

        const UdpConnectionPtr& connect(const udp::endpoint& remote);

        void sendEveryone(UdpPacketBase&& packet, bool reliable = false);
        
        
        void registerListener(uint16_t protocol, const UdpPacketListenerPtr& listener);

        void dispatchReceivedPackets();

    private:

        void startReceive();

        void handleReceive(const boost::system::error_code& error, size_t recvBytes);


        IOServicePtr m_ioservice;
        udp::endpoint m_localhost;
        SocketPtr m_socket;

        std::array<uint8_t, cMaxUdpPacketSize> m_recvBuf;
        udp::endpoint m_recvPeer;

        std::map<udp::endpoint, UdpConnectionPtr> m_connections;
        UdpPacketDispatcher m_dispatcher;
    };

}
