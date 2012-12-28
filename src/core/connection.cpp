#include "stdafx.h"
#include "core/connection.h"
#include "core/packet_dispatcher.h"
#include "core/ack_utils.h"
#include <boost/asio/placeholders.hpp>


namespace core {

    Connection::Connection(udp::socket& socket, const udp::endpoint& peer)
        : m_socket(socket), m_peer(peer), m_seqNum(0), m_ack(0), m_ackBits(0), m_errorCount(0)
    {}


    void Connection::send(const PacketPtr& packet, size_t resendLimit)
    {
        m_sent.push_front(PacketExt(packet, resendLimit, ++m_seqNum, m_ack, m_ackBits));

        m_socket.async_send_to(
            boost::asio::buffer(m_sent.front().packet->buffer()), m_peer,
            boost::bind(&Connection::handleSend, this, m_seqNum, boost::asio::placeholders::error));

        cDebug() << "sent packet" << m_seqNum << "with protocol" << packet->header().protocol;
    }


    // handler for logging errors during async_send_to
    void Connection::handleSend(uint16_t seqNum, const boost::system::error_code& error)
    {
        if (error)
        {
            cError() << "[send]" << error.category().name() << ":" << error.value() << ":" << error.message();

            m_sent.remove_if([&seqNum](const PacketExt& p){
                return p.header().seqNum == seqNum;
            });
        }
    }


    // usually we receive packets in order, so most recent come later
    // so it makes sense to search for insertion point from most recent to oldest
    void Connection::handleReceive(const PacketPtr& packet)
    {
        // process header
        const PacketHeader& header = packet->header();
        updateAcks(m_ack, m_ackBits, header.seqNum);
        processPeerAcks(header.ack, header.ackBits);

        // find first packet older than this one
        auto it = m_received.begin();
        for (; it != m_received.end(); ++it)
        {
            if (moreRecentPacket(*packet, **it))
                break;
        }

        m_received.insert(it, std::move(packet));
        m_errorCount = 0;
    }


    void Connection::processPeerAcks(uint16_t peerAck, uint32_t peerAckBits)
    {
        // peerAck = latest seqNum that peer received
        // peerAckBits = next 32 acks after latest

        // for each packet in queue, starting from most recent
        for (auto it = m_sent.begin(); it != m_sent.end();)
        {
            uint16_t seqNum = it->header().seqNum;
            if (moreRecentSeqNum(seqNum, peerAck))
            {
                // too young to receive ack
                ++it;
                continue;
            }

            uint16_t delta = peerAck - seqNum;
            if (delta > 32)
            {
                // this packet (it) is too old to receive ack, because all newer packets
                // in this connection will have more recent peer ack
                for (; it != m_sent.end();)
                {
                    if (it->resendLimit > 0)
                        send(it->packet, it->resendLimit - 1);
                    it = m_sent.erase(it);
                }
                break;
            }

            if (delta == 0)
            {
                // acknowledge this packet
                cDebug() << "acknowledged" << seqNum;
                it = m_sent.erase(it);
            }
            else
            {
                uint32_t bit = ackBitFromDelta(delta);
                if ((peerAckBits & bit) == bit)
                {
                    // acknowledge this packet
                    cDebug() << "acknowledged" << seqNum;
                    it = m_sent.erase(it);
                }
            }
        }
    }


    // dispatch all packets from oldest to most recent, to all active listeners
    void Connection::dispatchReceivedPackets(const PacketDispatcher& dispatcher)
    {
        // go in reverse, from oldest to most recent
        for (auto it = m_received.rbegin(); it != m_received.rend(); ++it)
            dispatcher.dispatchPacket(*this, *it);

        // clear receive queue
        m_received.clear();
    }

}
