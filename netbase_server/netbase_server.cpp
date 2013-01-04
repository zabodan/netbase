#include "stdafx.h"
#include "core/test_server.h"

using namespace core;
using namespace std::chrono;


int main(int argc, char **argv)
{
	std::locale::global(std::locale("rus"));
    LogService::ScopeGuard logGuard(&std::cout);

    try
    {
        IOServiceThread ioThread;

        const size_t maxTicks = argc > 1 ? atoi(argv[1]) : 1000;
        TestServer server(ioThread, 13999, maxTicks);
    }
    catch (const std::exception& ex)
    {
        LogError() << ex.what();
    }

	return 0;
}
