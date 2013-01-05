#include "stdafx.h"
#include "core/test_server.h"

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

        const size_t maxTicks = argc > 1 ? atoi(argv[1]) : 1000;
        TestServer server(ioThread, 13999, maxTicks);

        /*
        TestClient client(ioThread, 0, maxTicks);
        TestClient client2(ioThread, 0, maxTicks);
        //TestClient client3(ioThread, 0, maxTicks);
        //TestClient client4(ioThread, 0, maxTicks);
        */
    }
    catch (const std::exception& ex)
    {
        LogError() << ex.what();
    }

	return 0;
}
