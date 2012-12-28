#pragma once
#include <memory>
#include <map>


namespace core {

    class Connection;
    class Packet;
    typedef std::shared_ptr<Packet> PacketPtr;


    class IPacketListener
    {
    public:
        virtual ~IPacketListener() {}
        virtual void receive(const Connection& conn, const PacketPtr& packet) = 0;
    };

    typedef std::shared_ptr<IPacketListener> PacketListenerPtr;


    class PacketDispatcher
    {
    public:
        void registerListener(uint16_t protocol, const PacketListenerPtr& listener);
        void dispatchPacket(const Connection& conn, const PacketPtr& packet) const;

    private:
        // map protocol id to multiple listeners
        std::multimap<uint16_t, PacketListenerPtr> m_listeners;
    };

}
