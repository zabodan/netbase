#pragma once
#include "core/packet.h"
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

        void send(const PacketPtr& packet, size_t resendLimit = 0);

        bool isDead() const { return m_isDead; }

    protected:

        struct PacketExt
        {
            PacketExt(const PacketPtr& p, size_t resend, uint16_t seqNum, uint16_t ack, uint32_t ackBits)
                : packet(p), resendLimit(resend), timestamp(system_clock::now())
            {
                p->header().seqNum = seqNum;
                p->header().ack = ack;
                p->header().ackBits = ackBits;
            }

            const PacketHeader& header() const { return packet->header(); }

            PacketPtr packet;

            // how many times to try and resend this packet
            size_t resendLimit;

            // timestamp to measure latency
            system_clock::time_point timestamp;
        };

        friend class SmartSocket;

        // receive fresh new packet, place into m_received queue
        void handleReceive(const PacketPtr& packet);

        // dispatch all packets in m_received to all active listeners, then clear m_received
        void dispatchReceivedPackets(const PacketDispatcher& dispatcher);

        // handler for logging errors during async_send_to
        void handleSend(uint16_t seqNum, const boost::system::error_code& error);

        // clean up m_sent queue, confirm delivered packets and remove (or resend) old ones
        void processPeerAcks(uint16_t peerAck, uint32_t peerAckBits);

        // mark connection dead (to be removed later), or revive (if received any packets)
        void markDead(bool value) { m_isDead = value; }

        // remove or resend packet which was considered undelivered, it = m_sent.erase(it)
        void removeUndeliveredPacket(std::list<PacketExt>::iterator& it);

        // confirm packet, compute RTT, it = m_sent.erase(it)
        void confirmPacketDelivery(std::list<PacketExt>::iterator& it);

        SmartSocket& m_socket;
        udp::endpoint m_peer;
        bool m_isDead;

        uint16_t m_seqNum;
        uint16_t m_ack;         // most recently received peer seqNum
        uint32_t m_ackBits;
        uint16_t m_oldest;      // oldest not dispatched yet packet

        // packets awaiting aknowledge response, from most recent to old
        std::list<PacketExt> m_sent;

        // received packets placed here, recent go first
        std::vector<PacketPtr> m_received;
    };


    typedef std::shared_ptr<Connection> ConnectionPtr;


}
