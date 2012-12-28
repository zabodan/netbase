#include "stdafx.h"
#include "core/smart_socket.h"

using namespace core;


class ModP1 : public IPacketListener
{
public:

    ModP1() : received(false) {}

    bool received;

protected:

    void receive(const Connection& conn, const PacketPtr& packet)override
    {
        const PacketHeader& header = packet->header();
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
        SmartSocket socket(io_service, 13999);

        auto mod_p1 = std::make_shared<ModP1>();
        socket.registerListener(1, mod_p1);

        for (size_t tick = 0; ; ++tick)
        {
            io_service->poll();

            mod_p1->received = false;
            socket.dispatchReceivedPackets();

            if (mod_p1->received)
            {
                auto packet = std::make_shared<Packet>(2);
                socket.sendEveryone(packet);
            }

            // heartbit
            socket.sendEveryone(std::make_shared<Packet>(3));

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

