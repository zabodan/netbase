#include "stdafx.h"
#include "core/smart_socket.h"
#include "core/ioservice_thread.h"

using namespace core;
using namespace std::chrono;


class ModP1 : public IProtocolListener, public ISocketStateObserver
{
public:

    ModP1() : receivedCount(0) {}

    size_t receivedCount;

protected:

    void receive(const Connection& conn, const PacketPtr& packet) override
    {
        const PacketHeader& header = packet->header();
        cDebug << "processed packet" << header.seqNum << "with protocol" << header.protocol << "from" << conn.peer();
        ++receivedCount;
    }
};



int main(int argc, char **argv)
{
	std::locale::global(std::locale("rus"));
    LogService::ScopedGuard logGuard(&std::cout);

    try
    {
        IOServiceThread ioThread;

        auto socket = std::make_shared<SmartSocket>(ioThread.getService(), 13999);
        socket->addObserver(std::make_shared<SocketStateLogger>());

        ioThread.addResource(socket);

        auto mod_p1 = std::make_shared<ModP1>();
        socket->registerProtocolListener(1, mod_p1);

        size_t maxTicks = argc > 1 ? atoi(argv[1]) : 10000;
        for (size_t tick = 0; tick < maxTicks; ++tick)
        {
            auto ts1 = system_clock::now();

            mod_p1->receivedCount = 0;
            socket->dispatchReceivedPackets();

            if (mod_p1->receivedCount > 0)
            {
                auto packet = std::make_shared<Packet>(2);
                socket->sendEveryone(packet);
            }

            if (tick > 0 && tick % 10 == 0)
            {
                static const uint16_t cHeartBitProtocol = 3;
                socket->sendEveryone(std::make_shared<Packet>(cHeartBitProtocol));
            }

            auto ts2 = system_clock::now();
            auto work_duration = duration_cast<milliseconds>(ts2 - ts1);

            cDebug << "tick" << tick << "work done in" << work_duration;
            std::this_thread::sleep_for(milliseconds(50) - work_duration);
        }
    }
    catch (const std::exception& ex)
    {
        cError << ex.what();
    }

	return 0;
}

