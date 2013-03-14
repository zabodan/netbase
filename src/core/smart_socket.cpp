#include "stdafx.h"
#include "core/smart_socket.h"
#include "core/logger.h"
#include <boost/asio/placeholders.hpp>
#include <boost/bind.hpp>
#include <chrono>


namespace core {

    using namespace std::chrono;
    using namespace boost::asio;

    // housekeeping timer works with boost::chrono best
    static const auto cHouseKeepingPeriod = boost::chrono::seconds(1);
    static const auto cConnectionTimeout = std::chrono::seconds(5);


    SmartSocket::SmartSocket(const IOServicePtr& ioservice, size_t port)
      : m_ioservice(ioservice),
        m_localhost(udp::v4(), port),
        m_socket(*ioservice, m_localhost),
        m_housekeepTimer(*m_ioservice)
    {
        LogTrace() << "SmartSocket::SmartSocket";
        startReceive();

        m_housekeepTimer.expires_from_now(cHouseKeepingPeriod);
        m_housekeepTimer.async_wait(boost::bind(&SmartSocket::handleHouseKeep, this, placeholders::error));
    }

    SmartSocket::~SmartSocket()
    {
        notifyObservers(&ISocketStateObserver::onSocketShutdown);
        LogTrace() << "SmartSocket::~SmartSocket";
    }

    ConnectionPtr SmartSocket::getOrCreateConnection(const udp::endpoint& remote)
    {
        ConnectionPtr conn;
        if (!m_connections.find(remote, conn))
        {
            conn.reset(new Connection(*this, remote));
            m_connections.insert(remote, conn);
        }
        return conn;
    }

    ConnectionPtr SmartSocket::getExistingConnection(const udp::endpoint& remote)
    {
        ConnectionPtr conn;
        m_connections.find(remote, conn);
        return conn; // unchanged (nullptr) if not found
    }


    void SmartSocket::startReceive()
    {
        m_socket.async_receive_from(buffer(m_recvBuf), m_recvPeer,
            boost::bind(&SmartSocket::handleReceive, this, placeholders::error, placeholders::bytes_transferred));
    }


    void SmartSocket::handleReceive(const boost::system::error_code& error, size_t recvBytes)
    {
        try
        {
            if (error == error::message_size || (!error && recvBytes < sizeof(PacketHeader)))
            {
                notifyObservers(&ISocketStateObserver::onBadPacketSize, m_recvPeer, recvBytes);
            }
            else if (error)
            {
                auto conn = getExistingConnection(m_recvPeer);
                if (conn && !conn->isDead())
                    switch (error.value())
                    {
                        case error::connection_aborted:
                        case error::connection_refused:
                        case error::connection_reset:
                            notifyObservers(&ISocketStateObserver::onPeerDisconnect, conn);
                            conn->markDead(true);
                            break;
                
                        default:
                            notifyObservers(&ISocketStateObserver::onError, conn, error);
                            break;
                    }
            }
            else
            {
                ConnectionPtr conn = getOrCreateConnection(m_recvPeer);
                conn->handleReceive(std::make_shared<Packet>(m_recvBuf.data(), recvBytes));
                conn->markDead(false);
            }
        }
        catch (const std::exception& ex)
        {
            LogError() << ex.what();
        }
        catch (...)
        {
            LogFatal() << "unknown exception" << cSourceLocation;
        }
        startReceive();
    }


    void SmartSocket::registerProtocolListener(uint16_t protocol, const ProtocolListenerPtr& listener)
    {
        m_dispatcher.registerListener(protocol, listener);
    }


    void SmartSocket::dispatchReceivedPackets()
    {
        m_connections.for_each_value([&](const ConnectionPtr& conn){
            conn->dispatchReceivedPackets(m_dispatcher);
        });
    }


    void SmartSocket::sendEveryone(const PacketPtr& packet, size_t resendLimit)
    {
        m_connections.for_each_value([&](const ConnectionPtr& conn)
        {
            if (!conn->isDead())
                conn->asyncSend(std::make_shared<Packet>(*packet), resendLimit);
        });
    }


    void SmartSocket::handleHouseKeep(const boost::system::error_code& error)
    {
        if (error == boost::asio::error::operation_aborted)
        {
            LogDebug() << "HouseKeeping timer was aborted";
            return;
        }

        m_housekeepTimer.expires_at(m_housekeepTimer.expires_at() + cHouseKeepingPeriod);
        m_housekeepTimer.async_wait(boost::bind(&SmartSocket::handleHouseKeep, this, placeholders::error));

        // find timed out connections and mark them dead, and count dead connections
        // to determine if we need to remove anything (slow path that we want to avoid)
        auto timeoutStart = system_clock::now() - cConnectionTimeout;
        size_t deadCount = 0;

        m_connections.for_each_value([&](const ConnectionPtr& conn)
        {
            if (conn->lastActivityTime() < timeoutStart)
            {
                LogDebug() << "connection with" << conn->peer() << "timed out";
                conn->markDead(true);
            }

            if (conn->isDead())
                ++deadCount;
        });

        // remove dead connections -- slow path, but rare (write lock)
        if (deadCount > 0)
        {
            m_connections.remove_if([](const ConnectionPtr& conn){ return conn->isDead(); });
        }
    }
}