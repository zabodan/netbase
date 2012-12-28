#pragma once
#include "core/packet.h"
#include "core/logger.h"
#include <boost/bind.hpp>
#include <set>


namespace core {


    inline bool more_recent(uint16_t a, uint16_t b)
    {
        static const uint16_t mid = std::numeric_limits<uint16_t>::max() / 2;
        return a - b < mid;
    }

    inline bool more_recent(const Packet& p1, const Packet& p2)
    {
        return more_recent(p1.header().seqNum, p2.header().seqNum);
    }




    class PacketDispatcher;


    class Connection
    {
    public:

        Connection(udp::socket& socket, const udp::endpoint& peer);

        const udp::endpoint& peer() const { return m_peer; }

        void send(Packet&& packet, bool reliable = false);

    protected:

        friend class SmartSocket;

        // receive fresh new packet from network, place into m_received queue
        void handleReceive(Packet&& packet);

        // dispatch all packets in m_received to all active listeners, then clear m_received
        void dispatchReceivedPackets(const PacketDispatcher& dispatcher);

        // handler for logging errors during async_send_to
        void handleSend(uint16_t seqNum, const boost::system::error_code& error);

        void processHeader(const PacketHeader& header);

        void adjustMyAck(uint16_t seqNum);

        void processPeerAcks(uint16_t peerAck, uint32_t peerAckBits);

        udp::socket& m_socket;
        udp::endpoint m_peer;

        uint16_t m_seqNum;
        uint16_t m_ack;
        uint32_t m_ackBits;

        // packets awaiting aknowledge response, from most recent to old
        std::list<std::pair<Packet, bool>> m_sent;

        // received packets placed here, recent go first
        std::list<Packet> m_received;
    };


    typedef std::shared_ptr<Connection> ConnectionPtr;


}
