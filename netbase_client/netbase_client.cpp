#include "stdafx.h"
#include "core/test_client.h"

using namespace core;
using namespace std::chrono;


int main(int argc, char **argv)
{
	std::locale::global(std::locale("rus"));
    LogService::ScopeGuard logGuard(&std::cout);

	try
	{
        IOServiceThread ioThread;

        const size_t maxTicks = argc > 1 ? atoi(argv[1]) : 10;
        TestClient client(ioThread, 0, maxTicks);
        TestClient client2(ioThread, 0, maxTicks);
        //TestClient client3(ioThread, 0, maxTicks);
        //TestClient client4(ioThread, 0, maxTicks);
	}
	catch (const std::exception& ex)
	{
		LogError() << ex.what();
	}

	return 0;
}

