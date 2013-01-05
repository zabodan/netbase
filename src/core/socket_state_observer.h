#pragma once
#include "core/connection.h"
#include "core/logger.h"


namespace core {

    
    struct ISocketStateObserver
    {
        virtual ~ISocketStateObserver() {}

        // invoked after handshake when connection established
        virtual void onConnect(const ConnectionPtr&) {}

        // invoked when connection is closed by peer
        virtual void onPeerDisconnect(const ConnectionPtr&) {}

        // invoked when received too few (less then sizeof(PacketHeader)) or too much (more then 512 bytes) data
        virtual void onBadPacketSize(const udp::endpoint&, size_t) {}

        // invoked on connection errors
        virtual void onError(const ConnectionPtr&, const boost::system::error_code&) {}

        // invoked when socket is going to be destroyed
        virtual void onSocketShutdown() {}
    };



    class SocketStateLogger : public ISocketStateObserver
    {
    protected:

        void onConnect(const ConnectionPtr& conn)
        {
            LogInfo() << "connection established with" << conn->peer();
        }

        void onPeerDisconnect(const ConnectionPtr& conn)
        {
            LogInfo() << "peer" << conn->peer() << "disconnected";
        }

        void onBadPacketSize(const udp::endpoint& peer, size_t size)
        {
            LogError() << "received packet with bad size" << size << "from" << peer;
        }

        void onError(const ConnectionPtr& conn, const boost::system::error_code& error)
        {
            LogError() << "error on connection with" << conn->peer();
            LogError() << "  category:" << error.category().name() << "id:" << error.value() << "message:" << error.message();
        }

        virtual void onSocketShutdown()
        {
            LogInfo() << "socket is shutting down";
        }
    };


}
