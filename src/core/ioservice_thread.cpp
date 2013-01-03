#include "stdafx.h"
#include "core/ioservice_thread.h"
#include "core/logger.h"

void core::IOServiceThread::run()
{
    LogTrace() << "IOServiceThread: started ioservice event loop";
    
    while (!m_service->stopped())
    {
        try
        {
            m_service->run();
        }
        catch (const std::exception& ex)
        {
            LogFatal() << ex.what() << cSourceLocation;
        }
        catch (...)
        {
            LogFatal() << "unknown exception" << cSourceLocation;
        }
    }

    LogTrace() << "IOServiceThread: done";
}
