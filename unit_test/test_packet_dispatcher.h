#pragma once
#include "core/packet_dispatcher.h"
#include "core/iconnection.h"
#include "core/packet.h"
#include <boost/optional.hpp>


class TestConnection : public core::IConnection
{
public:

    TestConnection(const udp::endpoint& peer) : m_peer(peer)
    {}
    
    const udp::endpoint& peer() const override
    {
        return m_peer;
    }

private:
    const udp::endpoint m_peer;
};


class TestProtocolListener : public core::IProtocolListener
{
public:

    void receive(const core::IConnection& conn, const core::PacketPtr& packet) override
    {
        m_packet = packet;
    }

    bool tryRelease(core::PacketPtr& result)
    {
        if (m_packet)
        {
            result = *m_packet;
            m_packet.reset();
            return true;
        }
        return false;
    }

private:

    boost::optional<core::PacketPtr> m_packet;
};