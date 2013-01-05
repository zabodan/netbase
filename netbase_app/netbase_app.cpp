#include "stdafx.h"
#include "core/test_server.h"
#include "core/test_client.h"

using namespace core;
using namespace std::chrono;


class Config
{
public:

    Config()
    {
    }


private:


};




int main(int argc, char **argv)
{
	std::locale::global(std::locale("rus"));
    LogService::ScopeGuard logGuard(&std::cout);

    try
    {
        IOServiceThread ioThread;

        const size_t maxTicks = argc > 2 ? atoi(argv[2]) : 1000;
        const std::string mode = argc > 1 ? argv[1] : "server";

        if (mode == "server")
        {
            TestServer server(ioThread, 13999, maxTicks);
        }
        else
        {
            TestClient client(ioThread, 0, maxTicks);
            TestClient client2(ioThread, 0, maxTicks);
            //TestClient client3(ioThread, 0, maxTicks);
            //TestClient client4(ioThread, 0, maxTicks);
        }
    }
    catch (const std::exception& ex)
    {
        LogError() << ex.what();
    }

	return 0;
}
