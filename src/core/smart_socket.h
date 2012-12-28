#pragma once
#include "core/connection.h"
#include "core/packet_dispatcher.h"
#include <map>


namespace core {

    typedef std::shared_ptr<boost::asio::io_service> IOServicePtr;


    class SmartSocket
    {
    public:

        SmartSocket(const IOServicePtr& ioservice, size_t port);

        const ConnectionPtr& connect(const udp::endpoint& remote);

        void sendEveryone(Packet&& packet, bool reliable = false);


        void registerListener(uint16_t protocol, const PacketListenerPtr& listener);

        void dispatchReceivedPackets();

    private:

        void startReceive();

        void handleReceive(const boost::system::error_code& error, size_t recvBytes);


        IOServicePtr m_ioservice;
        udp::endpoint m_localhost;
        udp::socket m_socket;

        std::array<uint8_t, cMaxUdpPacketSize> m_recvBuf;
        udp::endpoint m_recvPeer;

        std::map<udp::endpoint, ConnectionPtr> m_connections;
        PacketDispatcher m_dispatcher;
    };

}

