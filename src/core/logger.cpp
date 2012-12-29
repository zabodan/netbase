#include "stdafx.h"
#include "core/logger.h"
#include <boost/format.hpp>
#include <chrono>
#include <iomanip>
#include <iostream>

namespace core
{

    using namespace std::chrono;

    std::ostream* LoggerBase::s_out = &std::cout;

    std::string timestamp()
    {
        auto now = system_clock::now();
        std::time_t t = system_clock::to_time_t(now);
        std::tm loc;
        localtime_s(&loc, &t);

        auto ms = duration_cast<milliseconds>(now.time_since_epoch());
        return boost::str(boost::format("%1%.%2$03d") % std::put_time(&loc, "%Y-%m-%d %H:%M:%S") % (ms.count() % 1000));
    }


    LoggerBase::~LoggerBase()
    {
        static const char* prefix[] = { "-N-", "-D-", "-I-", "-W-", "-E-", "-F-" };
        if (s_out && m_severity != None)
        {
            (*s_out) << timestamp() << " " << prefix[m_severity] << m_buffer.str() << "\n";
        }
    }
}