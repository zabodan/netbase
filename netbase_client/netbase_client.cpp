#include "stdafx.h"
#include "core/smart_socket.h"

using namespace core;


int main(int argc, char **argv)
{
	std::locale::global(std::locale("rus"));

	try
	{
		auto io_service = std::make_shared<boost::asio::io_service>();
        SmartSocket socket(io_service, 0);

		udp::resolver resolver(*io_service);
		udp::resolver::query serverQuery(udp::v4(), "localhost", "13999");

        auto conn = socket.connect(*resolver.resolve(serverQuery));

        for (size_t tick = 0; tick < 25; ++tick)
        {
            io_service->poll();

            if (conn->isBad())
                break;

            auto packet = std::make_shared<Packet>(1);
            conn->send(packet);

            cDebug() << "tick";
            boost::this_thread::sleep_for(boost::chrono::milliseconds(200));
        }
	}
	catch (const std::exception& e)
	{
		cError() << e.what();
	}

	return 0;
}

