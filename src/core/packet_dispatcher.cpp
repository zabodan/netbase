#include "stdafx.h"
#include "core/packet_dispatcher.h"
#include "core/iconnection.h"
#include "core/packet.h"


namespace core {

    // register another listener for specified protocol
    void PacketDispatcher::registerListener(uint16_t protocol, const ProtocolListenerPtr& listener)
    {
        m_listeners.insert(std::make_pair(protocol, listener));
    }


    // notify all listeners of this protocol
    void PacketDispatcher::dispatchPacket(const IConnection& conn, const PacketPtr& packet) const
    {
        uint16_t protocol = packet->header().protocol;
        auto it = m_listeners.lower_bound(protocol);
        auto ie = m_listeners.upper_bound(protocol);

        for (; it != ie; ++it)
        {
            it->second->receive(conn, packet);
        }
    }

}
