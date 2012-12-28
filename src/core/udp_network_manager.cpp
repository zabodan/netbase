#include "stdafx.h"
#include "core/udp_network_manager.h"
#include "core/logger.h"
#include <boost/asio/placeholders.hpp>


namespace core
{


    UdpNetworkManager::UdpNetworkManager(const IOServicePtr& ioservice, size_t port)
        : m_ioservice(ioservice), m_localhost(udp::v4(), port)
    {
        m_socket.reset(new udp::socket(*ioservice, m_localhost));
        startReceive();
    }


    const UdpConnectionPtr& UdpNetworkManager::connect(const udp::endpoint& remote)
    {
        UdpConnectionPtr& conn = m_connections[remote];
        if (!conn)
        {
            cInfo() << "connecting to" << remote;
            conn.reset(new UdpConnection(m_socket, remote));
        }
        return conn;
    }

    
    void UdpNetworkManager::startReceive()
    {
        m_socket->async_receive_from(
            boost::asio::buffer(m_recvBuf), m_recvPeer,
            boost::bind(&UdpNetworkManager::handleReceive, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
    }


    void UdpNetworkManager::handleReceive(const boost::system::error_code& error, size_t recvBytes)
    {
        if (error)
        {
            cError() << error.message();
            return;
        }

        if (recvBytes < sizeof(UdpPacketHeader))
        {
            cError() << "received packet is too small -- skipping it";
            return;
        }

        try
        {
            UdpConnectionPtr& conn = m_connections[m_recvPeer];
            if (!conn)
            {
                cInfo() << "detected new incoming connection from" << m_recvPeer;
                conn.reset(new UdpConnection(m_socket, m_recvPeer));
            }
            conn->handleReceive(UdpPacketBase(m_recvBuf.data(), recvBytes));
        }
        catch (const std::exception& ex)
        {
            cError() << ex.what();
        }
        startReceive();
    }


    void UdpNetworkManager::registerListener(uint16_t protocol, const UdpPacketListenerPtr& listener)
    {
        m_dispatcher.registerListener(protocol, listener);
    }


    void UdpNetworkManager::dispatchReceivedPackets()
    {
        for (auto it : m_connections)
        {
            it.second->dispatchReceivedPackets(m_dispatcher);
        }
    }

    void UdpNetworkManager::sendEveryone(UdpPacketBase&& packet, bool reliable)
    {
        for (auto it : m_connections)
        {
            it.second->send(UdpPacketBase(packet), reliable);
        }
    }

}
