#pragma once
#include "core/connection.h"
#include "core/packet_dispatcher.h"
#include "core/observable.h"
#include "core/socket_state_observer.h"
#include "core/concurrent_map.h"
#include <boost/signal.hpp>
#include <boost/asio/system_timer.hpp>
#include <map>


namespace core {

    typedef std::shared_ptr<boost::asio::io_service> IOServicePtr;
    typedef boost::asio::system_timer HouseKeepTimer;
    typedef ConcurrentMap<udp::endpoint, ConnectionPtr> ConnectionsMap;


    class SmartSocket : public Observable<ISocketStateObserver>
    {
    public:

        SmartSocket(const IOServicePtr& ioservice, size_t port);

        ~SmartSocket();

        // find existing connection or create new
        ConnectionPtr getOrCreateConnection(const udp::endpoint& remote);

        // return connection if exists or nullptr
        ConnectionPtr getExistingConnection(const udp::endpoint& remote);

        // send packet to all connected peers
        void sendEveryone(const PacketPtr& packet, size_t resendLimit = 0);


        void registerProtocolListener(uint16_t protocol, const ProtocolListenerPtr& listener);

        void dispatchReceivedPackets();

        // accessor to underlying socket
        udp::socket& rawSocket() { return m_socket; }

        const IOServicePtr& getIOService() const { return m_ioservice; }

    private:

        void startReceive();

        void handleReceive(const boost::system::error_code& error, size_t recvBytes);

        void handleHouseKeep(const boost::system::error_code& error);

        IOServicePtr m_ioservice;
        udp::endpoint m_localhost;
        udp::socket m_socket;

        std::array<uint8_t, cMaxUdpPacketSize> m_recvBuf;
        udp::endpoint m_recvPeer;

        ConnectionsMap m_connections;
        PacketDispatcher m_dispatcher;
        HouseKeepTimer m_housekeepTimer;
    };


}

