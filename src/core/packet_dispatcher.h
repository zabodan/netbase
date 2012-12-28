#pragma once

namespace core {

    class Connection;
    class Packet;


    class IPacketListener
    {
    public:
        virtual ~IPacketListener() {}
        virtual void receive(const Connection& conn, const Packet& packet) = 0;
    };

    typedef std::shared_ptr<IPacketListener> PacketListenerPtr;


    class PacketDispatcher
    {
    public:
        void registerListener(uint16_t protocol, const PacketListenerPtr& listener);
        void dispatchPacket(const Connection& conn, const Packet& packet) const;

    private:
        // map protocol id to multiple listeners
        std::multimap<uint16_t, PacketListenerPtr> m_listeners;
    };

}
