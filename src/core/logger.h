#pragma once
#include "core/concurrent_queue.h"
#include <boost/noncopyable.hpp>
#include <sstream>
#include <chrono>
#include <thread>


namespace core
{


    template <class T> struct duration_suffix;
    template <> struct duration_suffix<std::chrono::hours>        { static const char* value() { return "h"; } };
    template <> struct duration_suffix<std::chrono::minutes>      { static const char* value() { return "m"; } };
    template <> struct duration_suffix<std::chrono::seconds>      { static const char* value() { return "s"; } };
    template <> struct duration_suffix<std::chrono::milliseconds> { static const char* value() { return "ms"; } };
    template <> struct duration_suffix<std::chrono::microseconds> { static const char* value() { return "mks"; } };
    template <> struct duration_suffix<std::chrono::nanoseconds>  { static const char* value() { return "ns"; } };


    typedef std::chrono::system_clock::time_point SCTimePoint;


    enum LogSeverity
    {
        LogSeverity_Trace,
        LogSeverity_Debug,
        LogSeverity_Info,
        LogSeverity_Warning,
        LogSeverity_Error,
        LogSeverity_Fatal,
        LogSeverity_None
    };


    class LogBase : private boost::noncopyable
    {
    public:

        LogBase(LogSeverity severity)
            : m_severity(severity)
        {
        }

        ~LogBase();

        template <class T, class P>
        LogBase& operator<<(const std::chrono::duration<T,P>& value)
        {
            typedef std::chrono::duration<T,P> dtype;
            m_buffer << " " << value.count() << duration_suffix<dtype>::value();
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

        LogSeverity m_severity;
        std::ostringstream m_buffer;
    };


    class LogNone
    {
    public:
        template <class T>
        LogNone& operator<<(const T& value)
        {
            return *this;
        }
    };

#ifndef cLogLevel
#define cLogLevel LogSeverity_Debug
#endif


#if cLogLevel <= LogSeverity_Trace
#define cTrace   LogBase(LogSeverity_Trace)
#else
#define cTrace   LogNone()
#endif

#if cLogLevel <= LogSeverity_Debug
#define cDebug   LogBase(LogSeverity_Debug)
#else
#define cDebug   LogNone()
#endif

#if cLogLevel <= LogSeverity_Info
#define cInfo    LogBase(LogSeverity_Info)
#else
#define cInfo    LogNone()
#endif

#if cLogLevel <= LogSeverity_Warning
#define cWarning LogBase(LogSeverity_Warning)
#else
#define cWarning LogNone()
#endif

#if cLogLevel <= LogSeverity_Error
#define cError   LogBase(LogSeverity_Error)
#else
#define cError   LogNone()
#endif

#if cLogLevel <= LogSeverity_Fatal
#define cFatal   LogBase(LogSeverity_Fatal)
#else
#define cFatal   LogNone()
#endif


#define cWHERE __FILE__ << ":" << __LINE__ << "[" << __FUNCTION__ << "]"


    // io service, singleton interface, supposed to run only in one thread
    class LogService : private boost::noncopyable
    {
    public:

        static LogService& instance();

        void start(std::ostream* sink);
        void stop();
        void log(LogSeverity severity, const std::string& message, const SCTimePoint& tp);

        struct ScopedGuard
        {
            ScopedGuard(std::ostream* sink) { LogService::instance().start(sink); }
            ~ScopedGuard() { LogService::instance().stop(); }
        };

    private:

        LogService();
        void run();
        void processQueue();

        struct LogRecord
        {
            LogSeverity severity;
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
