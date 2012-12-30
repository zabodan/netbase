#include "stdafx.h"
#include "core/connection.h"
#include "core/smart_socket.h"
#include "core/ack_utils.h"
#include <boost/asio/placeholders.hpp>
#include <boost/bind.hpp>
#include <chrono>


namespace core {

    using namespace std::chrono;

    static const size_t cReceivedQueueSize = 256;

    Connection::Connection(SmartSocket& socket, const udp::endpoint& peer)
      : m_socket(socket),
        m_peer(peer),
        m_isDead(false),
        m_seqNum(0),
        m_ack(0),
        m_ackBits(0),
        m_oldest(0)
    {
        m_received.resize(cReceivedQueueSize, nullptr);
    }


    void Connection::send(const PacketPtr& packet, size_t resendLimit)
    {
        m_sent.push_front(PacketExt(packet, resendLimit, ++m_seqNum, m_ack, m_ackBits));

        m_socket.rawSocket().async_send_to(
            boost::asio::buffer(m_sent.front().packet->buffer()), m_peer,
            boost::bind(&Connection::handleSend, this, m_seqNum, boost::asio::placeholders::error));

        cDebug << "sent packet" << m_seqNum << "with protocol" << packet->header().protocol << "to" << m_peer;
    }


    // handler for logging errors during async_send_to
    void Connection::handleSend(uint16_t seqNum, const boost::system::error_code& error)
    {
        if (error)
        {
            m_socket.notifyObservers([&](ISocketStateObserver& observer){ observer.onError(shared_from_this(), error); });

            // find failed packet in m_sent queue
            auto it = std::find_if(m_sent.begin(), m_sent.end(), [&seqNum](const PacketExt& p){
                return p.header().seqNum == seqNum;
            });

            if (it != m_sent.end())
                removeUndeliveredPacket(it);
        }
    }

    void Connection::removeUndeliveredPacket(std::list<PacketExt>::iterator& it)
    {
        // if we really want this packet delivered, try again
        if (it->resendLimit > 0)
            send(it->packet, it->resendLimit - 1);
        it = m_sent.erase(it);
    }

    void Connection::confirmPacketDelivery(std::list<PacketExt>::iterator& it)
    {
        auto observedRTT = duration_cast<milliseconds>(system_clock::now() - it->timestamp);
        cDebug << "acknowledged packet" << it->header().seqNum << "for peer" << m_peer << "RTT is" << observedRTT;
        it = m_sent.erase(it);
    }


    // usually we receive packets in order, so most recent come later, so we search
    // for insertion point from most recent to oldest
    void Connection::handleReceive(const PacketPtr& packet)
    {
        // process header
        const PacketHeader& header = packet->header();
        updateAcks(m_ack, m_ackBits, header.seqNum);
        processPeerAcks(header.ack, header.ackBits);

        if (moreRecentSeqNum(m_oldest, header.seqNum))
            m_oldest = header.seqNum;

        m_received[header.seqNum % cReceivedQueueSize] = packet;

        //// find first packet older than this one
        //auto it = m_received.begin();
        //for (; it != m_received.end(); ++it)
        //{
        //    if (moreRecentPacket(*packet, **it))
        //        break;
        //}

        //m_received.insert(it, std::move(packet));
    }


    // clean up m_sent queue, confirm delivered packets and remove (or resend) old ones
    //   peerAck     - latest seqNum that peer received
    //   peerAckBits - next 32 acks after latest
    void Connection::processPeerAcks(uint16_t peerAck, uint32_t peerAckBits)
    {

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
                // this packet (it) is too old to receive ack, because all newer packets from this peer
                // will have more recent peer ack -- so we remove (or resend) it, and all older packets too
                for (; it != m_sent.end();)
                    removeUndeliveredPacket(it);

                break;
            }

            if (delta == 0)
            {
                // peerAck == seqNum, hooraay
                confirmPacketDelivery(it);
            }
            else
            {
                // delta <= 32, we should check peerAckBits
                uint32_t bit = ackBitFromDelta(delta);
                if ((peerAckBits & bit) == bit)
                    confirmPacketDelivery(it);
            }
        }
    }


    // dispatch all packets from oldest to most recent, to all active listeners
    void Connection::dispatchReceivedPackets(const PacketDispatcher& dispatcher)
    {
        //// go in reverse, from oldest to most recent
        //for (auto it = m_received.rbegin(); it != m_received.rend(); ++it)
        //    dispatcher.dispatchPacket(*this, *it);

        //// clear receive queue
        //m_received.clear();

        for ( ; m_oldest != m_ack + 1; ++m_oldest)
        {
            auto& ptr = m_received[m_oldest % cReceivedQueueSize];
            if (ptr)
            {
                dispatcher.dispatchPacket(*this, ptr);
                ptr.reset();
            }
        }
    }

}
