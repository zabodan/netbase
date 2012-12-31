#pragma once
#include "core/packet.h"
#include "core/packet_buffer.h"
#include "core/logger.h"
#include <set>


namespace core {


    class PacketDispatcher;
    class SmartSocket;


    class Connection : public std::enable_shared_from_this<Connection>
    {
    public:

        Connection(SmartSocket& socket, const udp::endpoint& peer);

        const udp::endpoint& peer() const { return m_peer; }

        // makes new job for ioservice: doSend(packet, resendLimit)
        void asyncSend(const PacketPtr& packet, size_t resendLimit = 0);

        bool isDead() const { return m_isDead; }

        // dispatch all received packets to all active listeners
        void dispatchReceivedPackets(const PacketDispatcher& dispatcher);

    protected:

        static const size_t cQueueSize = 1024;


        struct PacketExt
        {
            PacketExt();
            void reset(const PacketPtr& p, size_t resend, uint16_t seqNum, uint16_t ack, uint32_t ackBits);
            const PacketHeader& header() const { return packet->header(); }

            PacketPtr packet;
            size_t resendLimit;                 // how many times to try and resend this packet
            system_clock::time_point timestamp; // timestamp to measure latency
        };

        
        friend class SmartSocket;
        
        // this function only called from ioservice strands (ioservice thread)
        void doSend(const PacketPtr& packet, size_t resendLimit);

        // process packet headers, place packet into queue
        void handleReceive(const PacketPtr& packet);

        // handler for logging errors during async_send_to
        void handleSend(uint16_t seqNum, const boost::system::error_code& error);

        // clean up m_sent queue, confirm delivered packets and remove (or resend) old ones
        void processPeerAcks(uint16_t peerAck, uint32_t peerAckBits);

        // mark connection dead (to be removed later), or revive (if received any packets)
        void markDead(bool value) { m_isDead = value; }

        // remove or resend packet which was considered undelivered, it = m_sent.erase(it)
        void removeUndeliveredPacket(uint16_t seqNum);

        // confirm packet, compute RTT, it = m_sent.erase(it)
        void confirmPacketDelivery(uint16_t seqNum);

        PacketExt& getSentPacketExt(uint16_t seqNum) { return m_sent[seqNum % m_sent.size()]; }

        SmartSocket& m_socket;
        const udp::endpoint m_peer;
        std::atomic<bool> m_isDead;          // peer disconnected
        size_t m_averageRTT;

        uint16_t m_seqNum;
        uint16_t m_ack;         // most recently received peer seqNum
        uint32_t m_ackBits;
        
        // packets awaiting aknowledge response, from most recent to old
        std::vector<PacketExt> m_sent;

        // received packets are placed here, recent go first
        RecvPacketBuffer<cQueueSize> m_received;
    };


    typedef std::shared_ptr<Connection> ConnectionPtr;

}
