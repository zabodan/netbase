#pragma once
#include <sstream>
#include <chrono>

namespace core
{

    template <class T> struct duration_suffix;
    template <> struct duration_suffix<std::chrono::hours>        { static const char* value() { return "h"; } };
    template <> struct duration_suffix<std::chrono::minutes>      { static const char* value() { return "m"; } };
    template <> struct duration_suffix<std::chrono::seconds>      { static const char* value() { return "s"; } };
    template <> struct duration_suffix<std::chrono::milliseconds> { static const char* value() { return "ms"; } };
    template <> struct duration_suffix<std::chrono::microseconds> { static const char* value() { return "mks"; } };
    template <> struct duration_suffix<std::chrono::nanoseconds>  { static const char* value() { return "ns"; } };


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

        template <class T, class P>
        LoggerBase& operator<<(const std::chrono::duration<T,P>& value)
        {
            typedef std::chrono::duration<T,P> dtype;
            m_buffer << " " << value.count() << duration_suffix<dtype>::value();
            return *this;
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
