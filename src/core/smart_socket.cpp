#include "stdafx.h"
#include "core/smart_socket.h"
#include "core/logger.h"
#include <boost/asio/placeholders.hpp>
#include <boost/bind.hpp>


namespace core {


    SmartSocket::SmartSocket(const IOServicePtr& ioservice, size_t port)
        : m_ioservice(ioservice),
        m_localhost(udp::v4(), port),
        m_socket(*ioservice, m_localhost)
    {
        startReceive();
    }

    SmartSocket::~SmartSocket()
    {
        notifyObservers([](ISocketStateObserver& observer){ observer.onSocketShutdown(); });
    }

    ConnectionPtr SmartSocket::getOrCreateConnection(const udp::endpoint& remote)
    {
        ConnectionPtr& conn = m_connections[remote];
        if (!conn)
        {
            conn.reset(new Connection(*this, remote));
            //notifyObservers([&](ISocketStateObserver& observer){ observer.onConnect(conn); });
        }
        return conn;
    }

    ConnectionPtr SmartSocket::getExistingConnection(const udp::endpoint& remote)
    {
        auto it = m_connections.find(remote);
        return it != m_connections.end() ? it->second : nullptr;
    }


    void SmartSocket::startReceive()
    {
        m_socket.async_receive_from(
            boost::asio::buffer(m_recvBuf), m_recvPeer,
            boost::bind(&SmartSocket::handleReceive, this,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    }


    void SmartSocket::handleReceive(const boost::system::error_code& error, size_t recvBytes)
    {
        if (error == boost::asio::error::message_size || (!error && recvBytes < sizeof(PacketHeader)))
        {
            notifyObservers([=](ISocketStateObserver& observer){ observer.onBadPacketSize(m_recvPeer, recvBytes); });
        }
        else if (error)
        {
            auto conn = getExistingConnection(m_recvPeer);
            if (conn && !conn->isDead())
                switch (error.value())
                {
                    case boost::asio::error::connection_aborted:
                    case boost::asio::error::connection_refused:
                    case boost::asio::error::connection_reset:
                        notifyObservers([&](ISocketStateObserver& observer){ observer.onPeerDisconnect(conn); });
                        conn->markDead(true);
                        break;
                
                    default:
                        notifyObservers([&](ISocketStateObserver& observer){ observer.onError(conn, error); });
                        break;
                }
        }
        else try
        {
            ConnectionPtr conn = getOrCreateConnection(m_recvPeer);
            conn->handleReceive(std::make_shared<Packet>(m_recvBuf.data(), recvBytes));
            conn->markDead(false);
        }
        catch (const std::exception& ex)
        {
            cError() << ex.what();
        }
        startReceive();
    }


    void SmartSocket::registerProtocolListener(uint16_t protocol, const ProtocolListenerPtr& listener)
    {
        m_dispatcher.registerListener(protocol, listener);
    }


    void SmartSocket::dispatchReceivedPackets()
    {
        for (auto it : m_connections)
        {
            it.second->dispatchReceivedPackets(m_dispatcher);
        }
    }

    void SmartSocket::sendEveryone(const PacketPtr& packet, size_t resendLimit)
    {
        for (auto it : m_connections)
        {
            if (it.second->isDead())
                continue;

            auto clone = std::make_shared<Packet>(*packet);
            it.second->send(clone, resendLimit);
        }
    }

}