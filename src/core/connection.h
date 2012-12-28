#pragma once
#include "core/packet.h"
#include "core/logger.h"
#include <boost/bind.hpp>
#include <set>


namespace core {


    class PacketDispatcher;


    class Connection
    {
    public:

        Connection(udp::socket& socket, const udp::endpoint& peer);

        const udp::endpoint& peer() const { return m_peer; }

        void send(const PacketPtr& packet, size_t resendLimit = 0);

        bool isBad() const { return m_errorCount >= 2; }

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

        void processPeerAcks(uint16_t peerAck, uint32_t peerAckBits);

        void onError() { ++m_errorCount; }



        udp::socket& m_socket;
        udp::endpoint m_peer;

        uint16_t m_seqNum;
        uint16_t m_ack;
        uint32_t m_ackBits;

        // packets awaiting aknowledge response, from most recent to old
        std::list<PacketExt> m_sent;

        // received packets placed here, recent go first
        std::list<PacketPtr> m_received;

        size_t m_errorCount;
    };


    typedef std::shared_ptr<Connection> ConnectionPtr;


}
