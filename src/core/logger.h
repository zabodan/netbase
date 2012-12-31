#pragma once
#include "core/mpmc_queue.h"
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


    class LogBase : private boost::noncopyable
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

        LogBase(Severity severity)
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

        Severity m_severity;
        std::ostringstream m_buffer;
    };


#define cDebug   LogBase(LogBase::Debug)
#define cInfo    LogBase(LogBase::Info)
#define cWarning LogBase(LogBase::Warning)
#define cError   LogBase(LogBase::Error)
#define cFatal   LogBase(LogBase::Fatal)
#define cWHERE __FILE__ << ":" << __LINE__ << "[" << __FUNCTION__ << "]"


    // io service, singleton interface, supposed to run only in one thread
    class LogService : private boost::noncopyable
    {
    public:

        static LogService& instance();

        void start(std::ostream* sink);
        void stop();
        void log(LogBase::Severity severity, const std::string& message, const SCTimePoint& tp);

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
            LogBase::Severity severity;
            std::string message;
            SCTimePoint timestamp;

            void writeTo(std::ostream& out) const;
        };

        mpmc_queue<LogRecord> m_queue;
        std::ostream* m_sink;
        std::unique_ptr<std::thread> m_thread;
        std::atomic<bool> m_stopRequested;
        LogRecord m_record;
    };

}
