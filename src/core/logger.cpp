#include "stdafx.h"
#include "core/logger.h"

#include <iostream>

namespace core
{

    std::ostream* LoggerBase::s_out = &std::cout;

    LoggerBase::~LoggerBase()
    {
        static const char* prefix[] = { "-N-", "-D-", "-I-", "-W-", "-E-", "-F-" };
        if (s_out && m_severity != None)
        {
            (*s_out) << prefix[m_severity] << m_buffer.str() << std::endl;
        }
    }
}