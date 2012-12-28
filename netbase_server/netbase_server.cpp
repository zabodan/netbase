#include "stdafx.h"
#include "core/udp_network_manager.h"

using namespace core;


class ModP1 : public IUdpPacketListener
{
public:

    ModP1() : received(false) {}

    bool received;

protected:

    void receive(const UdpConnection& conn, const UdpPacketBase& packet)override
    {
        const auto& header = packet.header();
        cDebug() << "processed packet" << header.seqNum << "with protocol" << header.protocol << "from" << conn.peer();

        received = true;
    }
};

int main(int argc, char **argv)
{
	std::locale::global(std::locale("rus"));

    try
    {
        auto io_service = std::make_shared<boost::asio::io_service>();
        UdpNetworkManager network(io_service, 13999);

        auto mod_p1 = std::make_shared<ModP1>();
        network.registerListener(1, mod_p1);

        for (size_t tick = 0; ; ++tick)
        {
            io_service->poll();

            mod_p1->received = false;
            network.dispatchReceivedPackets();

            if (mod_p1->received)
            {
                UdpPacketBase response(2);
                network.sendEveryone(std::move(response));
            }

            cDebug() << "tick" << tick;
            boost::this_thread::sleep_for(boost::chrono::seconds(1));
        }
    }
    catch (const std::exception& ex)
    {
        cError() << ex.what();
    }

	return 0;
}

