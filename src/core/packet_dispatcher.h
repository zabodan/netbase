#pragma once
#include <memory>
#include <map>


namespace core {

    class Connection;
    class Packet;
    typedef std::shared_ptr<Packet> PacketPtr;


    struct IProtocolListener
    {
        virtual ~IProtocolListener() {}
        virtual void receive(const Connection& conn, const PacketPtr& packet) = 0;
    };

    typedef std::shared_ptr<IProtocolListener> ProtocolListenerPtr;


    class PacketDispatcher
    {
    public:
        void registerListener(uint16_t protocol, const ProtocolListenerPtr& listener);
        void dispatchPacket(const Connection& conn, const PacketPtr& packet) const;

    private:
        // map protocol id to multiple listeners
        std::multimap<uint16_t, ProtocolListenerPtr> m_listeners;
    };

}
