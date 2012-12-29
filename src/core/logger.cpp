#include "stdafx.h"
#include "core/logger.h"
#include <iomanip>

namespace core
{

    using namespace std::chrono;

    LogBase::~LogBase()
    {
        if (m_severity != None)
            LogService::instance().log(m_severity, m_buffer.str(), system_clock::now());
    }



    LogService::LogService()
        : m_sink(nullptr), m_stopRequested(false)
    {
    }

    //static
    LogService& LogService::instance()
    {
        static LogService service;
        return service;
    }

    void LogService::start(std::ostream* sink)
    {
        if (m_thread)
            throw std::runtime_error("LogService already running!");

        m_sink = sink;
        m_thread.reset(new std::thread([this]{ run(); }));
    }

    void LogService::stop()
    {
        if (m_thread)
        {
            m_stopRequested = true;
            m_thread->join();
            m_thread.reset();
        }
    }
    
    void LogService::log(LogBase::Severity severity, const std::string& message, const SCTimePoint& tp)
    {
        LogRecord record = { severity, message, tp };
        m_queue.push(std::move(record));
    }
    
    void LogService::run()
    {
        LogRecord record;
        while (!m_stopRequested)
        {
            for (size_t count = 0; m_queue.pop(record) && m_sink && count < 10; ++count)
            {
                record.writeTo(*m_sink);
            }
            std::this_thread::yield();
        }
        m_stopRequested = false;
    }


    void LogService::LogRecord::writeTo(std::ostream& out) const
    {
        static const char* prefix[] = { " -N-", " -D-", " -I-", " -W-", " -E-", " -F-" };

        // note: as long as we have single writing thread, static buffers are fine
        static std::time_t tt;
        static std::tm loc;
        
        // get local time in std::tm structure
        tt = system_clock::to_time_t(timestamp);
        localtime_s(&loc, &tt);

        // get milliseconds
        auto ms = duration_cast<milliseconds>(timestamp.time_since_epoch());

        out << std::put_time(&loc, "%Y-%m-%d %H:%M:%S")
            << std::setfill('0') << std::setw(3) << ms.count() % 1000 << std::setfill(' ')
            << prefix[severity] << message << "\n";
    }


}