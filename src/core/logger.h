#pragma once
#include "core/concurrent_queue.h"
#include <boost/noncopyable.hpp>
#include <memory>
#include <iosfwd>
#include <chrono>
#include <thread>


namespace core
{

    namespace sc = std::chrono;

    template <class P> struct duration_suffix;
    template <> struct duration_suffix<std::ratio<3600>> { static const char* value() { return "h"; } };
    template <> struct duration_suffix<std::ratio<60>>   { static const char* value() { return "m"; } };
    template <> struct duration_suffix<std::ratio<1>>    { static const char* value() { return "s"; } };
    template <> struct duration_suffix<std::milli>       { static const char* value() { return "ms"; } };
    template <> struct duration_suffix<std::micro>       { static const char* value() { return "mks"; } };
    template <> struct duration_suffix<std::nano>        { static const char* value() { return "ns"; } };

    struct set_fixed
    {
        set_fixed(size_t d) : digits(d) {}
        size_t digits;
    };


    typedef std::chrono::system_clock::time_point SCTimePoint;


    class LogBase : private boost::noncopyable
    {
    public:

        enum Severity
        {
            Trace,
            Debug,
            Info,
            Warning,
            Error,
            Fatal,
            None
        };


        LogBase(Severity severity)
            : m_severity(severity)
        {
        }

        virtual ~LogBase();

        LogBase& operator<<(const set_fixed& p)
        {
            m_buffer.precision(p.digits);
            m_buffer.setf(std::ios::fixed);
            return *this;
        }

        template <class T, class P>
        LogBase& operator<<(const std::chrono::duration<T,P>& value)
        {
            m_buffer << " " << value.count() << duration_suffix<P>::value();
            return *this;
        }

        template <class T>
        LogBase& operator<<(const T& value)
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
    };


    class LogNone : private boost::noncopyable
    {
    public:
        template <class T>
        LogNone& operator<<(const T& value)
        {
            return *this;
        }
    };


#ifndef MinLogLevel
#define MinLogLevel LogBase::Debug
#endif
    
#define cSourceLocation __FILE__ << ":" << __LINE__ << "[" << __FUNCTION__ << "]"


    template <int SL, class Enable = void>
    class Logger;
    
    template <int SL>
    class Logger<SL, typename std::enable_if< (SL >= MinLogLevel) >::type> : public LogBase
    {
    public:
        Logger() : LogBase(Severity(SL)) {}
    };

    template <int SL>
    class Logger<SL, typename std::enable_if< (SL < MinLogLevel) >::type> : public LogNone
    {
    };

    typedef Logger<LogBase::Trace>   LogTrace;
    typedef Logger<LogBase::Debug>   LogDebug;
    typedef Logger<LogBase::Info>    LogInfo;
    typedef Logger<LogBase::Warning> LogWarning;
    typedef Logger<LogBase::Error>   LogError;
    typedef Logger<LogBase::Fatal>   LogFatal;


    // log i/o service, singleton interface, must run only in one thread
    class LogService : private boost::noncopyable
    {
    public:

        static LogService& instance();

        void start(std::ostream* sink);
        void stop();
        void log(LogBase::Severity severity, const std::string& message, const SCTimePoint& tp);

        struct ScopeGuard
        {
            ScopeGuard(std::ostream* sink) { LogService::instance().start(sink); }
            ~ScopeGuard() { LogService::instance().stop(); }
        };

    private:

        LogService();
        void run();
        void processQueue();

        struct LogRecord
        {
            LogBase::Severity severity;
            std::string message;
            SCTimePoint timestamp;

            void writeTo(std::ostream& out) const;
        };

        mpsc_queue<LogRecord> m_queue;
        std::ostream* m_sink;
        std::unique_ptr<std::thread> m_thread;
        std::atomic<bool> m_stopRequested;
        LogRecord m_record;
    };

}
