#pragma once
#include <sstream>

namespace core
{

    class LoggerBase
    {
    public:

        enum Severity
        {
            None = 0,
            Debug,
            Info,
            Warning,
            Error,
            Fatal
        };

        LoggerBase(Severity severity)
            : m_severity(severity)
        {
        }

        LoggerBase(LoggerBase&& rhs)
            : m_severity(rhs.m_severity), m_buffer(std::move(rhs.m_buffer))
        {
            rhs.m_severity = None;
        }

        ~LoggerBase();

        static void SetLogStream(std::ostream* os)
        {
            s_out = os;
        }

        template <class T>
        LoggerBase& operator<<(const T& value)
        {
            write(value, std::is_integral<T>::type());
            return *this;
        }

    protected:

        template <class T>
        void write(const T& value, std::true_type& /*isIntegral*/)
        {
            m_buffer << " " << int64_t(value);
        }

        template <class T>
        void write(const T& value, std::false_type&)
        {
            m_buffer << " " << value;
        }

        Severity m_severity;
        std::ostringstream m_buffer;

        static std::ostream* s_out;
    };


    inline LoggerBase cDebug() { return LoggerBase(LoggerBase::Debug); }
    inline LoggerBase cInfo() { return LoggerBase(LoggerBase::Info); }
    inline LoggerBase cWarning() { return LoggerBase(LoggerBase::Warning); }
    inline LoggerBase cError() { return LoggerBase(LoggerBase::Error); }
    inline LoggerBase cFatal() { return LoggerBase(LoggerBase::Fatal); }

}
