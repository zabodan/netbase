#include "stdafx.h"
#include "core/connection.h"
#include "core/smart_socket.h"
#include "core/ack_utils.h"
#include <boost/asio/placeholders.hpp>
#include <boost/bind.hpp>
#include <chrono>


namespace core {

    using namespace std::chrono;

    
    Connection::PacketExt::PacketExt() : resendLimit(0)
    {
    }

    void Connection::PacketExt::reset(const PacketPtr& p, size_t resend, uint16_t seqNum, uint16_t ack, uint32_t ackBits)
    {
        packet = p;
        packet->header().seqNum = seqNum;
        packet->header().ack = ack;
        packet->header().ackBits = ackBits;
        resendLimit = resend;
        timestamp = system_clock::now();
    }
    
    
    Connection::Connection(SmartSocket& socket, const udp::endpoint& peer)
      : m_socket(socket),
        m_peer(peer),
        m_isDead(false),
        m_seqNum(0),
        m_ack(0),
        m_ackBits(0),
        m_averageRTT(50)
    {
        m_sent.resize(cQueueSize);
    }


    void Connection::asyncSend(const PacketPtr& packet, size_t resendLimit)
    {
        // note: lambda here catches packet by value!
        m_socket.getIOService()->post([=]{ doSend(packet, resendLimit); });
    }

    void Connection::doSend(const PacketPtr& packet, size_t resendLimit)
    {
        uint16_t seqNum = ++m_seqNum;

        PacketExt& pExt = getSentPacketExt(seqNum);

        if (pExt.packet)
        {
            cWarning << "send buffer is full on connection with" << m_peer;
            if (pExt.resendLimit > 0)
                asyncSend(pExt.packet, pExt.resendLimit - 1);
        }

        pExt.reset(std::move(packet), resendLimit, seqNum, m_ack, m_ackBits);

        m_socket.rawSocket().async_send_to(
            boost::asio::buffer(pExt.packet->buffer()), m_peer,
            boost::bind(&Connection::handleSend, this, seqNum, boost::asio::placeholders::error));

        cDebug << "sending packet" << seqNum << "with protocol" << pExt.packet->header().protocol << "to" << m_peer;
    }


    // handler for logging errors during async_send_to
    void Connection::handleSend(uint16_t seqNum, const boost::system::error_code& error)
    {
        if (error)
        {
            m_socket.notifyObservers([&](ISocketStateObserver& observer){ observer.onError(shared_from_this(), error); });
            removeUndeliveredPacket(seqNum);
        }
        cDebug << "sent" << seqNum;
    }

    void Connection::removeUndeliveredPacket(uint16_t seqNum)
    {
        PacketExt& pExt = getSentPacketExt(seqNum);
        if (pExt.resendLimit > 0)
            doSend(pExt.packet, pExt.resendLimit - 1);
        pExt.packet = nullptr;
    }

    void Connection::confirmPacketDelivery(uint16_t seqNum)
    {
        PacketExt& pExt = getSentPacketExt(seqNum);
        if (pExt.packet)
        {
            auto observedRTT = duration_cast<milliseconds>(system_clock::now() - pExt.timestamp);
            m_averageRTT = (9 * m_averageRTT + static_cast<size_t>(observedRTT.count())) / 10;
            
            cDebug << "acknowledged packet" << pExt.header().seqNum << "for peer" << m_peer << "RTT is" << observedRTT << "averageRTT" << m_averageRTT << "ms";
            pExt.packet = nullptr;
        }
    }


    // usually we receive packets in order, so most recent come later, so we search
    // for insertion point from most recent to oldest
    void Connection::handleReceive(const PacketPtr& packet)
    {
        // process header
        const PacketHeader& header = packet->header();
        updateAcks(m_ack, m_ackBits, header.seqNum);
        processPeerAcks(header.ack, header.ackBits);

        PacketPtr old = m_received.insert(header.seqNum, packet);
        if (old != nullptr)
        {
            if (old->header().seqNum == header.seqNum)
                cDebug << "detected packet duplicate";
            else
                cError << "recv buffer seems full, discarding old packet from" << m_peer;
        }
    }


    // clean up m_sent queue, confirm delivered packets and remove (or resend) old ones
    //   peerAck     - latest seqNum that peer received
    //   peerAckBits - next 32 acks after latest
    void Connection::processPeerAcks(uint16_t peerAck, uint32_t peerAckBits)
    {
        confirmPacketDelivery(peerAck);
        for (uint16_t delta = 1; delta <= 32; ++delta)
        {
            uint32_t bit = ackBitFromDelta(delta);
            if ((peerAckBits & bit) == bit)
                confirmPacketDelivery(uint16_t(peerAck - delta));
        }
    }


    // dispatch all packets from oldest to most recent, to all active listeners
    void Connection::dispatchReceivedPackets(const PacketDispatcher& dispatcher)
    {
        while (!m_received.empty())
        {
            PacketPtr packet = m_received.removeLast();
            if (packet)
                dispatcher.dispatchPacket(*this, packet);
        }
    }

}
