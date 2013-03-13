#pragma once
#include <memory>
#include <map>


namespace core {

    class IConnection;
    class Packet;
    typedef std::shared_ptr<Packet> PacketPtr;


    class IProtocolListener
    {
    public:
        virtual ~IProtocolListener() {}
        virtual void receive(const IConnection& conn, const PacketPtr& packet) = 0;
    };

    typedef std::shared_ptr<IProtocolListener> ProtocolListenerPtr;


    class PacketDispatcher
    {
    public:
        void registerListener(uint16_t protocol, const ProtocolListenerPtr& listener);
        void dispatchPacket(const IConnection& conn, const PacketPtr& packet) const;

    private:
        // map protocol id to multiple listeners
        std::multimap<uint16_t, ProtocolListenerPtr> m_listeners;
    };

}
