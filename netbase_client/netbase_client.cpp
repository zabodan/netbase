#include "stdafx.h"
#include "core/smart_socket.h"
#include "core/ioservice_thread.h"

using namespace core;
using namespace std::chrono;


int main(int argc, char **argv)
{
	std::locale::global(std::locale("rus"));
    LogService::ScopedGuard logGuard(&std::cout);

	try
	{
        IOServiceThread ioThread;

        auto socket = std::make_shared<SmartSocket>(ioThread.getService(), 0);
        socket->addObserver(std::make_shared<SocketStateLogger>());
        
        ioThread.addResource(socket);


        udp::resolver resolver(*ioThread.getService());
		udp::resolver::query serverQuery(udp::v4(), "localhost", "13999");
        udp::endpoint server = *resolver.resolve(serverQuery);

        auto conn = socket->getOrCreateConnection(server);

        cDebug << "client: start";
        size_t maxTicks = argc > 1 ? atoi(argv[1]) : 10;
        for (size_t tick = 0; tick < maxTicks; ++tick)
        {
            if (conn->isDead())
                break;

            socket->dispatchReceivedPackets();
            
            //if (tick < 20)
            {
                auto packet = std::make_shared<Packet>(1);
                conn->asyncSend(packet);
            }

            cDebug << "tick" << tick;
            std::this_thread::sleep_for(milliseconds(50));
        }
        cDebug << "client: done";
	}
	catch (const std::exception& e)
	{
		cError << e.what();
	}

	return 0;
}

