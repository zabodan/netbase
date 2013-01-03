#include "stdafx.h"
#include "core/connection.h"
#include "core/smart_socket.h"
#include "core/ack_utils.h"
#include <boost/asio/placeholders.hpp>
#include <boost/bind.hpp>
#include <chrono>


namespace core {

    using namespace std::chrono;

    
    Connection::Connection(SmartSocket& socket, const udp::endpoint& peer)
      : m_socket(socket),
        m_peer(peer),
        m_isDead(false),
        m_ack(0),
        m_ackBits(0),
        m_averageRTT(50),
        m_recvCount(0),
        m_sentCount(0),
        m_ackdCount(0)
    {
    }


    Connection::~Connection()
    {
        cDebug << "stats for" << m_peer
               << ": sent" << m_sentCount << "packets, confirmed" << m_ackdCount << "of them,"
               << "received" << m_recvCount << "packets, latest RTT was" << milliseconds(m_averageRTT);
    }


    void Connection::asyncSend(const PacketPtr& packet, size_t resendLimit)
    {
        // note: lambda here catches packet by value!
        m_socket.getIOService()->post([=]{ doSend(packet, resendLimit); });
    }


    void Connection::doSend(const PacketPtr& packet, size_t resendLimit)
    {
        // store packet in the send buffer, get previous value
        PacketExt old = m_sentPackets.store(packet, resendLimit, m_ack, m_ackBits);
        
        if (old.packet)
        {
            cWarning << "send buffer is full on connection with" << m_peer;
            if (old.resendLimit > 0)
                asyncSend(old.packet, old.resendLimit - 1);
        }

        uint16_t seqNum = packet->header().seqNum;
        m_socket.rawSocket().async_send_to(boost::asio::buffer(packet->buffer()), m_peer,
            boost::bind(&Connection::handleSend, this, packet, boost::asio::placeholders::error));

        cDebug << "sending packet" << seqNum << "with protocol" << packet->header().protocol << "to" << m_peer;
        ++m_sentCount;
    }


    // handler for logging errors during async_send_to
    void Connection::handleSend(const PacketPtr& packet, const boost::system::error_code& error)
    {
        cTrace << "[+] Connection::handleSend";
        if (error)
        {
            m_socket.notifyObservers([&](ISocketStateObserver& observer){ observer.onError(shared_from_this(), error); });
            removeUndeliveredPacket(packet->header().seqNum);
        }
        cTrace << "[-] Connection::handleSend";
    }

    void Connection::removeUndeliveredPacket(uint16_t seqNum)
    {
        if (m_sentPackets.contains(seqNum))
        {
            PacketExt pExt = m_sentPackets.release(seqNum);
            if (pExt.resendLimit > 0)
                doSend(pExt.packet, pExt.resendLimit - 1);
        }
    }

    void Connection::confirmPacketDelivery(uint16_t seqNum)
    {
        if (m_sentPackets.contains(seqNum))
        {
            PacketExt pExt = m_sentPackets.release(seqNum);

            milliseconds observedRTT = duration_cast<milliseconds>(system_clock::now() - pExt.timestamp);
            m_averageRTT = (9 * m_averageRTT + static_cast<size_t>(observedRTT.count())) / 10;
            
            cDebug << "acknowledged packet" << pExt.packet->header().seqNum << "for peer" << m_peer
                   << "RTT is" << observedRTT << "averageRTT" << milliseconds(m_averageRTT);
            ++m_ackdCount;
        }
    }


    // usually we receive packets in order, so most recent come later, so we search
    // for insertion point from most recent to oldest
    void Connection::handleReceive(const PacketPtr& packet)
    {
        cTrace << "[+] Connection::handleReceive";

        m_recvTime = system_clock::now();
        ++m_recvCount;

        const PacketHeader& header = packet->header();
        updateAcks(m_ack, m_ackBits, header.seqNum);
        processPeerAcks(header.ack, header.ackBits);

        PacketPtr old = m_recvPackets.insert(header.seqNum, packet);
        if (old != nullptr)
        {
            if (old->header().seqNum == header.seqNum)
                cDebug << "received packet" << header.seqNum << "duplicate from" << m_peer;
            else
                cError << "recv buffer seems full, discarding old packet from" << m_peer;
        }

        cTrace << "[-] Connection::handleReceive";
    }


    // clean up sent buffer, confirm delivered packets and remove (or resend) old ones
    //   peerAck     - latest seqNum that peer received
    //   peerAckBits - next 32 acks after latest
    void Connection::processPeerAcks(uint16_t peerAck, uint32_t peerAckBits)
    {
        confirmPacketDelivery(peerAck);
        for (uint16_t delta = 1; delta <= 32; ++delta)
        {
            uint32_t bit = ackBitFromDelta(delta);
            if ((peerAckBits & bit) == bit)
                confirmPacketDelivery(peerAck - delta);
        }

        // consider oldest packet undelivered if its seqNum is less then peerAck - 256,
        // or timestamp is more then two seconds away from current time
        const auto minTime = system_clock::now() - seconds(2);
        const uint16_t minSeqNum = peerAck - 256;

        while (!m_sentPackets.empty())
        {
            if (moreRecentSeqNum(minSeqNum, m_sentPackets.oldestSeqNum()) ||
                minTime > m_sentPackets.oldestTime())
            {
                removeUndeliveredPacket(m_sentPackets.oldestSeqNum());
            }
            else break;
        }
    }


    // dispatch all packets from oldest to most recent, to all active listeners
    void Connection::dispatchReceivedPackets(const PacketDispatcher& dispatcher)
    {
        while (!m_recvPackets.empty())
        {
            PacketPtr packet = m_recvPackets.removeLast();
            if (packet)
                dispatcher.dispatchPacket(*this, packet);
        }
    }

}
