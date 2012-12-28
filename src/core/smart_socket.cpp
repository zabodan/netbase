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
            cError() << error.message();
            return;
        }

        if (recvBytes < sizeof(PacketHeader))
        {
            cError() << "received packet is too small -- skipping it";
            return;
        }

        try
        {
            ConnectionPtr& conn = m_connections[m_recvPeer];
            if (!conn)
            {
                cInfo() << "detected new incoming connection from" << m_recvPeer;
                conn.reset(new Connection(m_socket, m_recvPeer));
            }
            conn->handleReceive(Packet(m_recvBuf.data(), recvBytes));
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

    void SmartSocket::sendEveryone(Packet&& packet, bool reliable)
    {
        for (auto it : m_connections)
        {
            it.second->send(Packet(packet), reliable);
        }
    }

}