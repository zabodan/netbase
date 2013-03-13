#pragma once
#include "core/iconnection.h"
#include "core/packet.h"
#include "core/packet_buffer.h"
#include "core/fast_spinlock.h"
#include <set>


namespace core {


    class PacketDispatcher;
    class SmartSocket;


    class Connection :
        public IConnection,
        public std::enable_shared_from_this<Connection>
    {
    public:

        Connection(SmartSocket& socket, const udp::endpoint& peer);
        ~Connection();

        // Implements IConnection::peer
        const udp::endpoint& peer() const override { return m_peer; }

        // makes new job for ioservice: doSend(packet, resendLimit)
        void asyncSend(const PacketPtr& packet, size_t resendLimit = 0);

        bool isDead() const { return m_isDead; }

        // dispatch all received packets to all active listeners
        void dispatchReceivedPackets(const PacketDispatcher& dispatcher);

        const SCTimePoint& lastActivityTime() const { return m_recvTime; }

    protected:

        friend class SmartSocket;
        
        // [io-thread-handle] store packet in send buffer, call async_send_to
        void doSend(const PacketPtr& packet, size_t resendLimit);

        // [io-thread-handle] success/failure handle for async_send_to
        void handleSend(const PacketPtr& packet, const boost::system::error_code& error);

        // [io-thread-handle] process packet headers, place packet into queue
        void handleReceive(const PacketPtr& packet);

        // clean up send buffer, confirm delivered packets and remove (or resend) old ones
        void processPeerAcks(const ack_type& peerAck);

        // remove or resend packet which was considered undelivered
        void removeUndeliveredPacket(uint16_t seqNum);

        // confirm packet, compute RTT, remove from send buffer
        void confirmPacketDelivery(uint16_t seqNum);

        // mark connection dead (to be removed later), or revive (if received any packets)
        void markDead(bool value) { m_isDead = value; }


        static const size_t cQueueSize = 1024;


        // owner of this connection
        SmartSocket& m_socket;

        // remote address of this connection
        const udp::endpoint m_peer;
        
        // peer disconnected
        std::atomic<bool> m_isDead;

        // average round-trip time
        size_t m_averageRTT;
        size_t m_recvCount;
        size_t m_sentCount;
        size_t m_ackdCount;

        // time when received last packet
        SCTimePoint m_recvTime;

        // acknowledgements for received packets
        ack_type m_ack;
        
        // packets awaiting aks response
        SendPacketBuffer<cQueueSize> m_sentPackets;

        // received packets, ready to be dispatched
        RecvPacketBuffer<cQueueSize> m_recvPackets;
    };


    typedef std::shared_ptr<Connection> ConnectionPtr;

}
