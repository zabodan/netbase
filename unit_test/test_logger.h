#pragma once
#include "core/logger.h"


class TestLogger : public core::Logger<core::LogBase::Debug>
{
public:
    ~TestLogger() override {}
    
    std::string release()
    {
        std::string rv = m_buffer.str();
        m_buffer.str("");
        return std::move(rv);
    }
};
