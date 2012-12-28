#include "stdafx.h"
#include "core/smart_socket.h"
#include "core/logger.h"
#include <boost/asio/placeholders.hpp>


namespace core {


    SmartSocket::SmartSocket(const IOServicePtr& ioservice, size_t port)
        : m_ioservice(ioservice),
        m_localhost(udp::v4(), port),
        m_socket(*ioservice, m_localhost)
    {
        startReceive();
    }


    const ConnectionPtr& SmartSocket::connect(const udp::endpoint& remote)
    {
        ConnectionPtr& conn = m_connections[remote];
        if (!conn)
        {
            cInfo() << "connecting to" << remote;
            conn.reset(new Connection(m_socket, remote));
        }
        return conn;
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
        if (error)
        {
            cError() << "[recv]" << error.category().name() << ":" << error.value() << ":" << error.message();
            auto it = m_connections.find(m_recvPeer);
            if (it != m_connections.end())
            {
                it->second->onError();
                if (it->second->isBad())
                {
                    cInfo() << "connection with" << m_recvPeer << "lost";
                }
            }
        }
        else if (recvBytes < sizeof(PacketHeader))
        {
            cError() << "received packet is too small -- skipping it";
        }
        else try
        {
            ConnectionPtr& conn = m_connections[m_recvPeer];
            if (!conn)
            {
                cInfo() << "detected new incoming connection from" << m_recvPeer;
                conn.reset(new Connection(m_socket, m_recvPeer));
            }
            auto packet = std::make_shared<Packet>(m_recvBuf.data(), recvBytes);
            conn->handleReceive(packet);
        }
        catch (const std::exception& ex)
        {
            cError() << ex.what();
        }
        startReceive();
    }


    void SmartSocket::registerListener(uint16_t protocol, const PacketListenerPtr& listener)
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
            if (it.second->isBad())
                continue;

            auto clone = std::make_shared<Packet>(*packet);
            it.second->send(clone, resendLimit);
        }
    }

}