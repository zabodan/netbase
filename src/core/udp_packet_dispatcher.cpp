#include "stdafx.h"
#include "core/udp_packet_dispatcher.h"
#include "core/udp_connection.h"


namespace core
{

    // register another listener for specified protocol
    void UdpPacketDispatcher::registerListener(uint16_t protocol, const UdpPacketListenerPtr& listener)
    {
        m_listeners.insert(std::make_pair(protocol, listener));
    }


    // notify all listeners of this protocol
    void UdpPacketDispatcher::dispatchPacket(const UdpConnection& conn, const UdpPacketBase& packet) const
    {
        uint16_t protocol = packet.header().protocol;
        auto it = m_listeners.lower_bound(protocol);
        auto ie = m_listeners.upper_bound(protocol);

        for (; it != ie; ++it)
        {
            it->second->receive(conn, packet);
        }
    }

}
