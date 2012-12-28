#pragma once

namespace core
{

    class UdpConnection;
    class UdpPacketBase;


    class IUdpPacketListener
    {
    public:
        virtual ~IUdpPacketListener() {}
        virtual void receive(const UdpConnection& conn, const UdpPacketBase& packet) = 0;
    };

    typedef std::shared_ptr<IUdpPacketListener> UdpPacketListenerPtr;


    class UdpPacketDispatcher
    {
    public:
        void registerListener(uint16_t protocol, const UdpPacketListenerPtr& listener);
        void dispatchPacket(const UdpConnection& conn, const UdpPacketBase& packet) const;

    private:
        // map protocol id to multiple listeners
        std::multimap<uint16_t, UdpPacketListenerPtr> m_listeners;
    };

}
